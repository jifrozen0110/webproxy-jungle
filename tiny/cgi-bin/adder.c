/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
   char* buf; // 환경변수 QUERY_STRING을 가리킬 포인터
   char* p; // & 문자를 가리킬 포인터
   char arg1[MAXLINE], arg2[MAXLINE]; // arg1 arg2 URL에서 추출한 두 숫자를 문자열로 저장할 배열
   char content[MAXLINE]; // content : 클라이언트에게 전송할 응답 본문을 저장할 배열
   int n1 = 0, n2 = 0; // n1, n2 : URL에서 추출한 두 숫자를 정수로 변환하여 저장할 변수

   /*
    * QUERY_STRING 환경변수에서 URL 파라미터를 추출
    * QUERY_STRING은 URL의 쿼리 문자열을 의미하며, CGI 프로그램에 전달된 파라미터가 저장됩니다.
    * 예: URL이 "http://example.com/cgi-bin/adder?"라면, QUERY_STRING은 "3&5"가 됩니다.
    */
   if ((buf = getenv("QUERY_STRING")) != NULL) { // getenv 함수를 사용하여 QUERY_STRING환경 변수를 가져옵니다.
      p = strchr(buf, '&'); // & 문자를 찾아서 두 숫자 사이를 구분
      *p = '\0'; // & 문자를 \0로 바꿔 문자열을 두 부분으로 분리
      strcpy(arg1, buf);
      strcpy(arg2, p + 1);
      n1 = atoi(strchr(arg1, '=') + 1);
      n2 = atoi(strchr(arg2, '=') + 1);
   }


   /*
    * 옹답 본문(content)을 작성
    * 클라이언트에게 보낼 HTML 형식의 응답 본문을 작성합니다.
    */
   sprintf(content, "QUERY_STRING=%s", buf);
   sprintf(content, "Welcome to add.com: ");
   sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
   sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
      content, n1, n2, n1 + n2);
   sprintf(content, "%sThanks for visiting!\r\n", content);

   /*
    * HTTP 응답 생성 및 전송
    * 클라이언트에게 전송할 HTTP 응답을 작성하고 출력합니다.
    */
   printf("Connection: close\r\n");
   printf("Content-length: %d\r\n", (int)strlen(content));
   printf("Content-type: text/html\r\n\r\n");
   printf("%s", content);
   fflush(stdout); // 출력 버퍼를 비워 클라이언트로 즉시 전송

   exit(0);
}
/* $end adder */
