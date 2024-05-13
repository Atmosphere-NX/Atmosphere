FROM devkitpro/devkita64:latest

WORKDIR /app/

COPY ./requirements.txt /opt/requirements.txt

RUN apt-get install -y --no-install-recommends python3-venv && \
python3 -m venv /opt/venv && \
. /opt//venv/bin/activate && \
pip install -r /opt/requirements.txt

RUN dkp-pacman -S --needed --noconfirm \
switch-glm switch-libjpeg-turbo \
devkitarm-rules devkitA64 hactool
