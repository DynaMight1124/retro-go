FROM espressif/idf:v5.5.1

WORKDIR /app

ADD . /app

# Apply patches
RUN cd /opt/esp/idf && \
	(patch --ignore-whitespace -p1 -i "/app/tools/patches/panic-hook (esp-idf 5).diff" || true) && \
	(patch --ignore-whitespace -p1 -i "/app/tools/patches/sdcard-fix (esp-idf 5).diff" || true)

SHELL ["/bin/bash", "-c"]

