#!/bin/sh

# Start the Docker daemon in the background
dockerd-entrypoint.sh &

# Capture the PID of the Docker daemon
DOCKER_PID=$!

# Function to gracefully stop Docker daemon on container shutdown
cleanup() {
    echo "Stopping Docker daemon..."
    kill $DOCKER_PID
    wait $DOCKER_PID
    exit 0
}

# Trap SIGTERM and SIGINT to allow graceful shutdown
trap cleanup SIGTERM SIGINT

# Wait until Docker daemon is ready
echo "Waiting for Docker daemon to be ready..."
while ! docker info > /dev/null 2>&1; do
    sleep 1
done

echo "Docker daemon is up and running."

# Execute the init script
echo "Running init script..."
cd /root/challenge
docker build -t challenge .

/root/private_instance_manager/InstancesServerApp

# Keep the container running by waiting for the Docker daemon process
wait $DOCKER_PID
