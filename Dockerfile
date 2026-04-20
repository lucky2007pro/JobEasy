FROM drogonframework/drogon:latest AS builder
WORKDIR /app
COPY . .
RUN cmake -B build . && cmake --build build --config Release

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libjsoncpp-dev uuid-dev zlib1g-dev libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Copy the binary and config
COPY --from=builder /app/build/JobEasy /app/JobEasy
COPY --from=builder /app/config.json /app/config.json

# Expose app port 
EXPOSE 8848

# Run
CMD ["./JobEasy"]
