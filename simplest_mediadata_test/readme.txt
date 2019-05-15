
// udp-rtp test project
ffmpeg -re -i sintel.ts -strict -2 -f rtp_mpegts udp://192.168.80.128:8880
./simplest_mediadata_test udp-rtp

// flv-parser test project
./simplest_mediadata_test flv-parser



