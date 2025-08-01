export class UIManager {
    constructor(robotController) {
        this.robotController = robotController;
        this.cameraInterval = null;
    }

    init() {
        this.updateConnectionStatus(false);
        this.setupCameraFeed();
    }

    updateConnectionStatus(connected) {
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');
        const connectBtn = document.getElementById('connectBtn');

        if (connected) {
            statusIndicator.className = 'status-indicator connected';
            statusText.textContent = 'Connected';
            connectBtn.textContent = 'Disconnect';
        } else {
            statusIndicator.className = 'status-indicator';
            statusText.textContent = 'Disconnected';
            connectBtn.textContent = 'Connect';
        }
    }

    setupCameraFeed() {
        const cameraFeed = document.getElementById('cameraFeed');
        cameraFeed.style.display = 'block';
    }

    startCamera() {
        if (!this.robotController.isConnected()) {
            alert('Please connect to the robot first');
            return;
        }

        const cameraFeed = document.getElementById('cameraFeed');
        const ip = document.getElementById('robotIP').value;
        
        // Set camera stream URL (port 81 for stream server)
        cameraFeed.src = `http://${ip}:81/stream`;
        cameraFeed.style.display = 'block';

        // Handle image load errors
        cameraFeed.onerror = () => {
            console.error('Failed to load camera stream');
            cameraFeed.src = '';
            alert('Failed to start camera stream. Please check the connection.');
        };

        cameraFeed.onload = () => {
            console.log('Camera stream started');
        };
    }

    stopCamera() {
        const cameraFeed = document.getElementById('cameraFeed');
        cameraFeed.src = '';
    }

    async updateSensorData() {
        try {
            await this.robotController.updateSensorData();
            const data = this.robotController.getSensorData();

            // Update UI elements
            document.getElementById('ultrasonicValue').textContent = data.ultrasonic.toFixed(1);
            document.getElementById('batteryValue').textContent = data.battery.toFixed(2);
            document.getElementById('trackingL').textContent = data.lineTracking.L;
            document.getElementById('trackingM').textContent = data.lineTracking.M;
            document.getElementById('trackingR').textContent = data.lineTracking.R;

            // Update tracking sensor colors based on values
            this.updateTrackingSensorColors(data.lineTracking);
        } catch (error) {
            console.error('Failed to update sensor display:', error);
        }
    }

    updateTrackingSensorColors(tracking) {
        const threshold = 500; // Adjust based on your sensor calibration
        
        ['L', 'M', 'R'].forEach(sensor => {
            const element = document.getElementById(`tracking${sensor}`);
            const value = tracking[sensor];
            
            if (value > threshold) {
                element.style.backgroundColor = '#ef4444'; // Red for line detected
                element.style.color = 'white';
            } else {
                element.style.backgroundColor = '#10b981'; // Green for no line
                element.style.color = 'white';
            }
        });
    }

    showNotification(message, type = 'info') {
        // Create a simple notification system
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.textContent = message;
        
        notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 1rem 1.5rem;
            border-radius: 8px;
            color: white;
            font-weight: 500;
            z-index: 1000;
            animation: slideIn 0.3s ease;
        `;

        switch (type) {
            case 'success':
                notification.style.backgroundColor = '#10b981';
                break;
            case 'error':
                notification.style.backgroundColor = '#ef4444';
                break;
            case 'warning':
                notification.style.backgroundColor = '#f59e0b';
                break;
            default:
                notification.style.backgroundColor = '#3b82f6';
        }

        document.body.appendChild(notification);

        // Remove after 3 seconds
        setTimeout(() => {
            notification.style.animation = 'slideOut 0.3s ease';
            setTimeout(() => {
                document.body.removeChild(notification);
            }, 300);
        }, 3000);
    }
}

// Add CSS for notifications
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from { transform: translateX(100%); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }
    
    @keyframes slideOut {
        from { transform: translateX(0); opacity: 1; }
        to { transform: translateX(100%); opacity: 0; }
    }
`;
document.head.appendChild(style);