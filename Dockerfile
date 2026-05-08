FROM gcc:14

WORKDIR /app

COPY . .

RUN gcc -Iinclude src/*.c -o server -pthread

EXPOSE 10000

CMD ["./server"]
