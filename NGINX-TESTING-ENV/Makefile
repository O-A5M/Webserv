.PHONY: help setup start stop restart logs status clean rebuild test shell

# Default target
help:
	@echo "NGINX Docker Development Environment"
	@echo "====================================="
	@echo "Available targets:"
	@echo "  start     - Start NGINX container"
	@echo "  stop      - Stop NGINX container"
	@echo "  restart   - Restart NGINX container"
	@echo "  logs      - Show container logs (follow)"
	@echo "  status    - Show container status"
	@echo "  test      - Test NGINX configuration"
	@echo "  shell     - Open shell in NGINX container"
	@echo "  rebuild   - Rebuild and restart container"
	@echo "  clean     - Stop and remove containers/volumes"
	@echo "  reload    - Reload NGINX configuration"



# Start containers
start:
	@echo "Starting NGINX container..."
	@docker-compose up -d

# Stop containers
stop:
	@echo "Stopping NGINX container..."
	@docker-compose down
	@echo "✓ NGINX stopped"

# Restart containers
restart:
	@echo "Restarting NGINX container..."
	@docker-compose restart nginx
	@echo "✓ NGINX restarted"

# Show logs
logs:
	@echo "Showing NGINX logs (Ctrl+C to exit)..."
	@docker-compose logs -f nginx

# Show container status
status:
	@echo "Container status:"
	@docker-compose ps

# Test NGINX configuration
test:
	@echo "Testing NGINX configuration..."
	@docker-compose exec nginx nginx -t
	@echo "✓ Configuration test complete"

# Open shell in container
shell:
	@echo "Opening shell in NGINX container..."
	@docker-compose exec nginx sh

# Rebuild container
rebuild:
	@echo "Rebuilding NGINX container..."
	@docker-compose down
	@docker-compose up -d --build
	@echo "✓ NGINX rebuilt and started"

# Clean up everything
clean:
	@echo "Cleaning up containers and volumes..."
	@docker-compose down -v
	@docker system prune -f
	@echo "✓ Cleanup complete"


# Reload NGINX configuration
reload:
	@echo "Reloading NGINX configuration..."
	@docker-compose exec nginx nginx -s reload
	@echo "✓ Configuration reloaded"

# Show container info
info:
	@echo "Container Information:"
	@echo "====================="
	@docker-compose ps --format "table {{.Name}}\t{{.Status}}\t{{.Ports}}"
	@echo ""
	@echo "Resource Usage:"
	@docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}"

# Update NGINX image
update:
	@echo "Updating NGINX image..."
	@docker-compose pull nginx
	@docker-compose up -d nginx
	@echo "✓ NGINX updated"