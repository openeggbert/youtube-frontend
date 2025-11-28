# youtube-frontend

ffmpeg -i "$video"mkv -preset slow -crf 18 "$video"webm

ffmpeg -i "$video"mkv -preset slow -crf 18 -vf scale="-1:480" "$video"webm

## Development
```aiignore
apt install libopencv-dev
apt install nlohmann-json3-dev
apt install libavformat-dev libavcodec-dev libavutil-dev
apt install libcurl4-openssl-dev

```
