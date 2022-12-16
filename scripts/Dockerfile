FROM alpine

RUN apk update && apk add \
    bash \
    qemu \
    qemu-system-riscv32 \
    qemu-system-riscv64

RUN mkdir -p /host

WORKDIR /host

ENTRYPOINT ["/host/scripts/rv6432qemu.sh"]
