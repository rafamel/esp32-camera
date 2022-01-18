/* HTTP Status */
#define HTTP_200 "200 OK"
#define HTTP_302 "302 Found"
#define HTTP_400 "400 Bad Request"
#define HTTP_404 "404 Not Found"
#define HTTP_408 "408 Request Timeout"
#define HTTP_500 "500 Internal Server Error"

/* API */
#define API_ERROR "{ \"error\": true }"
#define API_SUCCESS "{ \"success\": true }"

/* Streams */
#define STREAM_PART_BOUNDARY "123456789000000000000987654321"
#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=" STREAM_PART_BOUNDARY
#define STREAM_BOUNDARY "\r\n--" STREAM_PART_BOUNDARY "\r\n"
#define STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"
