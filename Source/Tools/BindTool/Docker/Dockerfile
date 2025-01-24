FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

COPY scripts/install-cmake.sh /tmp/
RUN chmod +x /tmp/install-cmake.sh \
  && /tmp/install-cmake.sh 3.21.7 \
  && rm -rf /tmp/install-cmake.sh /var/lib/apt/lists/*
ENV PATH="/opt/cmake/bin:${PATH}"

COPY setup.sh /tmp/
RUN chmod +x /tmp/setup.sh \
  && /tmp/setup.sh \
  && rm -rf /tmp/setup.sh /var/lib/apt/lists/*
