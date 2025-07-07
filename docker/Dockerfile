############################################################
# docker/Dockerfile ── Rocky Linux 8 + GCC 13 (C++20)
#   • 透過 build-arg 控制 install_weak_deps
############################################################

################ Stage 1 : build ################
FROM rockylinux:8 AS build

# ↓↓↓ 新增可選 build 參數（true / false）
ARG INSTALL_WEAK_DEPS=true

RUN dnf -y update && \
    dnf -y install epel-release dnf-plugins-core && \
    dnf config-manager --set-enabled powertools && \
    dnf -y --setopt=install_weak_deps=${INSTALL_WEAK_DEPS} \
           install gcc-toolset-13 gcc-toolset-13-gcc-c++ json-devel && \
    dnf clean all && rm -rf /var/cache/dnf

WORKDIR /build
COPY include/ ./include/
COPY src/     ./src/

RUN . /opt/rh/gcc-toolset-13/enable && \
    g++ -std=c++20 -O2 -I./include src/*.cpp -o Fault_simulator.exe && \
    strip Fault_simulator.exe

################ Stage 2 : runtime ##############
FROM rockylinux:8

ARG INSTALL_WEAK_DEPS=true   # 同樣帶入第二階段（供 dnf 使用）

RUN dnf -y update && \
    dnf -y install epel-release && \
    dnf -y --setopt=install_weak_deps=${INSTALL_WEAK_DEPS} \
           install python3 python3-pip gcc-toolset-13-runtime && \
    dnf clean all && rm -rf /var/cache/dnf

WORKDIR /app
ENV INPUT_DIR=/input \
    OUTPUT_DIR=/output

COPY --from=build /build/Fault_simulator.exe ./Fault_simulator.exe
COPY python/ ./python/
RUN pip3 install --no-cache-dir -r python/requirements.txt

ENTRYPOINT bash -c '\
  ./Fault_simulator.exe "$INPUT_DIR/fault.json" "$INPUT_DIR/March-LSD.json" \
                        "$OUTPUT_DIR/March-LSD_detection_report.txt" && \
  python3 python/txt2excel.py "$OUTPUT_DIR/March-LSD_detection_report.txt" \
                              "$OUTPUT_DIR/March-LSD_detection_report.xlsx" \
'
