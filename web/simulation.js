class Path {
    constructor() {
        this.positions = [];
        this.amplitude = { real: 0, imag: 0 };
        this.action = 0;
    }
}

class PathIntegralSimulation {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');

        // Simulation parameters (defaults) - optimized for performance
        this.params = {
            numPaths: 500,  // Reduced from 1000 for better performance
            timeSteps: 30,  // Reduced from 50 for better performance
            hbar: 1.0,
            mass: 1.0,
            dt: 0.1,
            dx: 0.1,
            startPos: -2.0,
            endPos: 2.0
        };

        this.paths = [];
        this.currentFrame = 0;
        this.totalTime = 0.0;
        this.isPaused = false;
        this.lastTime = 0;
        this.frameCount = 0;

        this.setupCanvas();
        this.generatePaths();
        this.setupControls();
        this.animate();
    }

    setupCanvas() {
        // Set canvas size to match display size
        const rect = this.canvas.getBoundingClientRect();
        this.canvas.width = rect.width * window.devicePixelRatio;
        this.canvas.height = rect.height * window.devicePixelRatio;
        this.ctx.scale(window.devicePixelRatio, window.devicePixelRatio);

        // Handle resize
        window.addEventListener('resize', () => {
            const rect = this.canvas.getBoundingClientRect();
            this.canvas.width = rect.width * window.devicePixelRatio;
            this.canvas.height = rect.height * window.devicePixelRatio;
            this.ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
        });
    }

    // Potential function (harmonic oscillator)
    V(x) {
        return 0.5 * x * x;
    }

    // Calculate action for a path
    calculateAction(positions) {
        let action = 0.0;
        for (let t = 1; t < positions.length; t++) {
            const dx = positions[t] - positions[t - 1];
            const kinetic = 0.5 * this.params.mass * dx * dx / (this.params.dt * this.params.dt);
            const potential = this.V(positions[t]);
            action += (kinetic - potential) * this.params.dt;
        }
        return action;
    }

    // Generate a random path using Monte Carlo
    generateRandomPath(x0, xf) {
        const path = new Array(this.params.timeSteps + 1);
        path[0] = x0;
        path[this.params.timeSteps] = xf;

        // Linear interpolation as starting guess
        for (let t = 1; t < this.params.timeSteps; t++) {
            const alpha = t / this.params.timeSteps;
            path[t] = (1 - alpha) * x0 + alpha * xf;
            // Add random fluctuation
            path[t] += (Math.random() - 0.5) * 2 * 0.5;
        }

        return path;
    }

    generatePaths() {
        this.paths = [];
        const x0 = this.params.startPos;
        const xf = this.params.endPos;

        for (let i = 0; i < this.params.numPaths; i++) {
            const path = new Path();
            path.positions = this.generateRandomPath(x0, xf);
            path.action = this.calculateAction(path.positions);

            // Calculate quantum amplitude
            const phase = -path.action / this.params.hbar;
            path.amplitude = {
                real: Math.cos(phase),
                imag: Math.sin(phase)
            };

            this.paths.push(path);
        }

        // Normalize amplitudes
        let sumReal = 0, sumImag = 0;
        for (const path of this.paths) {
            sumReal += path.amplitude.real;
            sumImag += path.amplitude.imag;
        }

        for (const path of this.paths) {
            path.amplitude.real /= sumReal;
            path.amplitude.imag /= sumImag;
        }
    }

    update() {
        if (this.isPaused) return;

        this.currentFrame++;
        this.totalTime += 0.016; // ~60 FPS

        // Regenerate paths less frequently for better performance
        if (this.currentFrame % 300 === 0) { // Changed from 120 to 300
            this.generatePaths();
        }
    }

    render() {
        const width = this.canvas.width / window.devicePixelRatio;
        const height = this.canvas.height / window.devicePixelRatio;

        // Clear canvas
        this.ctx.fillStyle = '#000000';
        this.ctx.fillRect(0, 0, width, height);

        // Set up coordinate system
        const margin = 60;
        const plotWidth = width - 2 * margin;
        const plotHeight = height - 2 * margin;

        // Transform to plot coordinates
        this.ctx.save();
        this.ctx.translate(margin, margin);

        // Draw coordinate axes
        this.ctx.strokeStyle = '#666666';
        this.ctx.lineWidth = 1;
        this.ctx.beginPath();
        // X-axis
        this.ctx.moveTo(0, plotHeight / 2);
        this.ctx.lineTo(plotWidth, plotHeight / 2);
        // Y-axis (time)
        this.ctx.moveTo(plotWidth / 2, 0);
        this.ctx.lineTo(plotWidth / 2, plotHeight);
        this.ctx.stroke();

        // Draw grid
        this.ctx.strokeStyle = '#333333';
        this.ctx.lineWidth = 0.5;
        for (let i = 0; i <= 10; i++) {
            const x = (i / 10) * plotWidth;
            const y = (i / 10) * plotHeight;

            this.ctx.beginPath();
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, plotHeight);
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(plotWidth, y);
            this.ctx.stroke();
        }

        // Draw potential well visualization
        this.ctx.strokeStyle = '#4A90E2';
        this.ctx.lineWidth = 2;
        this.ctx.beginPath();
        for (let i = 0; i <= plotWidth; i++) {
            const x = (i / plotWidth - 0.5) * 10; // Scale to [-5, 5]
            const potential = this.V(x);
            const px = i;
            const py = plotHeight - (potential * 20 + plotHeight * 0.1); // Scale and offset
            if (i === 0) {
                this.ctx.moveTo(px, py);
            } else {
                this.ctx.lineTo(px, py);
            }
        }
        this.ctx.stroke();

        // Optimized path rendering - limit number of paths drawn
        const maxPathsToDraw = Math.min(this.paths.length, 200); // Limit to 200 paths for performance
        const pathStep = Math.max(1, Math.floor(this.paths.length / maxPathsToDraw));

        // Batch similar colors together
        const colorGroups = new Map();

        for (let i = 0; i < this.paths.length; i += pathStep) {
            const path = this.paths[i];

            // Color based on amplitude magnitude and phase
            const magnitude = Math.sqrt(path.amplitude.real * path.amplitude.real +
                path.amplitude.imag * path.amplitude.imag);
            const phase = Math.atan2(path.amplitude.imag, path.amplitude.real);

            // Map phase to color (simplified)
            const r = 0.5 + 0.5 * Math.cos(phase);
            const g = 0.5 + 0.5 * Math.cos(phase + 2 * Math.PI / 3);
            const b = 0.5 + 0.5 * Math.cos(phase + 4 * Math.PI / 3);

            // Scale by magnitude
            const alpha = Math.min(magnitude * 10, 1.0);

            const colorKey = `${Math.floor(r * 255)},${Math.floor(g * 255)},${Math.floor(b * 255)},${alpha.toFixed(2)}`;

            if (!colorGroups.has(colorKey)) {
                colorGroups.set(colorKey, []);
            }
            colorGroups.get(colorKey).push(path);
        }

        // Draw paths by color groups for better performance
        for (const [colorKey, paths] of colorGroups) {
            const [r, g, b, alpha] = colorKey.split(',').map(Number);
            this.ctx.strokeStyle = `rgba(${r}, ${g}, ${b}, ${alpha})`;
            this.ctx.lineWidth = 1;

            // Draw all paths with this color at once
            for (const path of paths) {
                this.ctx.beginPath();
                for (let t = 0; t < path.positions.length; t += 2) { // Skip every other point for performance
                    const x = (path.positions[t] / 5 + 0.5) * plotWidth;
                    const y = (t / (path.positions.length - 1)) * plotHeight;
                    if (t === 0) {
                        this.ctx.moveTo(x, y);
                    } else {
                        this.ctx.lineTo(x, y);
                    }
                }
                this.ctx.stroke();
            }
        }

        // Draw start and end points
        this.ctx.fillStyle = '#FF0000';
        this.ctx.beginPath();
        const startX = (this.params.startPos / 5 + 0.5) * plotWidth;
        const startY = 0;
        const endX = (this.params.endPos / 5 + 0.5) * plotWidth;
        const endY = plotHeight;
        this.ctx.arc(startX, startY, 4, 0, 2 * Math.PI);
        this.ctx.arc(endX, endY, 4, 0, 2 * Math.PI);
        this.ctx.fill();

        this.ctx.restore();

        // Draw text info
        this.ctx.fillStyle = '#FFFFFF';
        this.ctx.font = '14px Arial';
        this.ctx.fillText(`Paths: ${this.params.numPaths} (showing ${maxPathsToDraw})`, 10, 20);
        this.ctx.fillText(`Time Steps: ${this.params.timeSteps}`, 10, 40);
        this.ctx.fillText(`ℏ: ${this.params.hbar}`, 10, 60);
        this.ctx.fillText(`Mass: ${this.params.mass}`, 10, 80);

        this.ctx.font = '12px Arial';
        this.ctx.fillText('Red: Start/End | Blue: Harmonic Potential | Colors: Path Amplitudes', 10, height - 10);
    }

    setupControls() {
        // Setup slider controls
        const sliders = [
            { id: 'numPaths', param: 'numPaths', step: 100 },
            { id: 'timeSteps', param: 'timeSteps', step: 5 },
            { id: 'hbar', param: 'hbar', step: 0.1 },
            { id: 'mass', param: 'mass', step: 0.1 },
            { id: 'dt', param: 'dt', step: 0.01 },
            { id: 'dx', param: 'dx', step: 0.01 },
            { id: 'startPos', param: 'startPos', step: 0.1 },
            { id: 'endPos', param: 'endPos', step: 0.1 }
        ];

        sliders.forEach(slider => {
            const element = document.getElementById(slider.id);
            const valueDisplay = document.getElementById(slider.id + 'Value');

            element.addEventListener('input', (e) => {
                const value = parseFloat(e.target.value);
                this.params[slider.param] = value;
                valueDisplay.textContent = value.toFixed(slider.step < 1 ? 2 : 0);
                this.generatePaths();
            });
        });

        // Setup buttons
        document.getElementById('regenerateBtn').addEventListener('click', () => {
            this.generatePaths();
        });

        document.getElementById('pauseBtn').addEventListener('click', () => {
            this.isPaused = !this.isPaused;
            document.getElementById('pauseBtn').textContent = this.isPaused ? 'Resume' : 'Pause';
        });
    }

    animate(currentTime = 0) {
        // Calculate FPS
        if (this.lastTime > 0) {
            const fps = 1000 / (currentTime - this.lastTime);
            document.getElementById('fpsDisplay').textContent = Math.round(fps);
        }
        this.lastTime = currentTime;

        this.frameCount++;
        document.getElementById('frameCount').textContent = this.frameCount;

        this.update();
        this.render();

        requestAnimationFrame((time) => this.animate(time));
    }
}

// Initialize simulation when page loads
document.addEventListener('DOMContentLoaded', () => {
    const canvas = document.getElementById('simulationCanvas');
    new PathIntegralSimulation(canvas);
});  