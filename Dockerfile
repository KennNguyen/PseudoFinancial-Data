FROM python:3.11-slim

# System deps
RUN apt update && apt install -y g++ cmake libeigen3-dev

# Set working directory
WORKDIR /usr/src/app

# Copy everything
COPY . .

# Build C++ binaries
RUN mkdir build && cd build && cmake .. && make && \
    cp factor_model heston_model app/web/static/

# Install Python deps
RUN pip install --no-cache-dir -r requirements.txt

# Expose port for FastAPI
EXPOSE 8000

# Run FastAPI
CMD ["uvicorn", "app.web.main:app", "--host", "0.0.0.0", "--port", "8000"]
