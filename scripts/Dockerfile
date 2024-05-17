FROM alpine:edge

RUN apk add qemu=9.0.0-r0

RUN apk update && apk add \
    bash \
    qemu \
    qemu-system-riscv32 \
    qemu-system-riscv64

RUN mkdir -p /host

WORKDIR /host

ENTRYPOINT ["/host/scripts/rv6432qemu.sh"]
