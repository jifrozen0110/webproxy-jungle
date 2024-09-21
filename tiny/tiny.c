/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize,char * method);
void get_filetype(char* filename, char* filetype);
void serve_dynamic(int fd, char* filename, char* cgiargs,char * method);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg,
    char* longmsg);
void echo(int connfd);
// argc는 명령어 인자 개수
// argv는 명령어 인자 배열
// 이 프로그램은 명령어 인자로 포트 번호를 받아서 서버 실행

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN); // Sigpipe 에러가 발생하면 EPIPE 에러를 반환
    // 서버 소켓, 클라이언트와의 연결 소켓
    int listenfd, connfd;
    // 클라이언트 호스트 이름, 포트
    char hostname[MAXLINE], port[MAXLINE];

    // 클라이언트 주소 길이
    socklen_t clientlen;

    // 클라이언트 주소 정보를 저장하는 구조체
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    // 명령어 인자가 2개가 아니면 (즉, 프로그램 이름과 포트 번호)
    // 잘못된 사용법임을 출력하고, 프로그램 종료
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 주어진 포트에서 소켓 열어 클라이언트 요청을 대기
    listenfd = Open_listenfd(argv[1]);

    while (1) { // 무한으로 클라이언트 요청을 지속적으로 받음
        // 클라이언트의 주소 크기
        clientlen = sizeof(clientaddr);
        // 클라이언트 연결 수락
        connfd = Accept(listenfd, (SA*)&clientaddr,
            &clientlen);
        // Getnameinfo 클라이언트 주소 구조체를 통해
        // 클라이언트의 호스트 이름과 포트 번호 가져옴
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
            0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        // 실제로 클라이언트와 통신하거나 요청을 처리하는 함수
        doit(connfd);
        // 연결 종료
        Close(connfd);
    }
}

void doit(int fd)
{
    int is_static; // 요청된 컨텐츠가 정적인지 동적인지 구분하는 변수 (1이면 정적 컨텐츠, 0이면 동적 컨텐츠)
    struct stat sbuf; // 파일 상태 정보를 저장하는 구조체 (파일 크기, 권한, 타입 등의 정보를 담음)
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 요청 라인에서 HTTP 메서드, URI, 버전 정보를 저장하는 문자열 변수들
    char filename[MAXLINE], cgiargs[MAXLINE]; // 파일 이름과 CGI 프로그램 인자 정보를 저장하는 변수들
    rio_t rio; // RIO 버퍼 구조체, 클라이언트와의 데이터를 버퍼링하면서 입출력 처리하기 위한 버퍼

    /* 1. 클라이언트의 요청 라인과 헤더를 읽기 위한 준비 */
    Rio_readinitb(&rio, fd); // 클라이언트의 파일 디스크립터(fd)를 기반으로 RIO 버퍼를 초기화함
    Rio_readlineb(&rio, buf, MAXLINE); // 요청 라인의 첫 번째 줄을 읽어서 buf에 저장 (GET /index.html HTTP/1.1 같은 요청 라인)
    printf("Request headers:\n");
    printf("%s", buf); // 요청 라인의 내용을 출력 (메서드, URI, 버전)

    /* 2. 요청 라인에서 HTTP 메서드, URI, 버전을 파싱 */
    sscanf(buf, "%s %s %s", method, uri, version); // 요청 라인의 첫 번째 줄에서 HTTP 메서드, URI, 버전 정보를 추출하여 각각 method, uri, version 변수에 저장


    if ((strcasecmp(method, "GET")) && (strcasecmp(method, "HEAD"))) { // HTTP 메서드가 GET인지 확인 (GET만 지원함, 다른 메서드는 지원하지 않음)
        clienterror(fd, method, "501", "Not implemented",
            "Tiny does not implement this method"); // GET이 아닌 메서드일 경우 501 Not Implemented 에러를 클라이언트에 응답
        return;
    }

    /* 3. 클라이언트로부터 요청 헤더를 계속해서 읽기 */
    read_requesthdrs(&rio); // HTTP 요청 헤더를 읽고, 필요시 로깅 또는 처리할 수 있음 (현재는 요청 헤더를 단순히 무시함)

    /* 4. URI를 분석하여 파일 이름과 CGI 인자를 구분 */
    is_static = parse_uri(uri, filename, cgiargs); // URI를 분석하여 요청된 컨텐츠가 정적 컨텐츠인지 동적 컨텐츠인지 구분 (is_static 변수에 저장)
    // is_static이 1이면 정적 컨텐츠, 0이면 동적 컨텐츠
    if (stat(filename, &sbuf) < 0) { // URI로 지정된 파일이 실제로 존재하는지 확인. stat() 함수는 파일의 상태 정보를 sbuf에 저장
        clienterror(fd, filename, "404", "Not found",
            "Tiny couldn’t find this file"); // 파일이 존재하지 않으면 404 Not Found 에러를 클라이언트에 응답
        return;
    }

    /* 5. 요청된 컨텐츠가 정적인 경우 처리 */
    if (is_static) { /* Serve static content */
        // 파일이 일반 파일이고 읽기 권한이 있는지 확인 (정적 컨텐츠는 일반 파일이어야 하며 읽기 가능해야 함)
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                "Tiny couldn’t read the file"); // 파일이 정규 파일이 아니거나 읽기 권한이 없으면 403 Forbidden 에러를 클라이언트에 응답
            return;
        }
        serve_static(fd, filename, sbuf.st_size,method); // 정적 파일을 클라이언트에 제공 (파일의 내용을 읽어 클라이언트에 전송)
    }
    /* 6. 요청된 컨텐츠가 동적인 경우 처리 */
    else { /* Serve dynamic content */
        // 파일이 일반 파일이고 실행 권한이 있는지 확인 (동적 컨텐츠는 CGI 프로그램이므로 실행 가능해야 함)
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                "Tiny couldn’t run the CGI program"); // CGI 프로그램이 정규 파일이 아니거나 실행 권한이 없으면 403 Forbidden 에러를 클라이언트에 응답
            return;
        }
        serve_dynamic(fd, filename, cgiargs,method); // CGI 프로그램을 실행하여 동적 컨텐츠를 클라이언트에 제공 (프로그램 실행 결과를 클라이언트에 전송)
    }
}
void clienterror(int fd, char* cause, char* errnum,
    char* shortmsg, char* longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    //Build the HTTP response body
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    //Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t* rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

int parse_uri(char* uri, char* filename, char* cgiargs) {
    char* ptr;

    if (!strstr(uri, "cgi-bin")) { /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri) - 1] == '/') {
            strcat(filename, "home.html");
        }
        return 1;
    }
    else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        }
        else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char* filename, int filesize,char * method) {

    int srcfd; // 파일 디스크립터를 저장할 변수
    char* srcp; // 메모리 매핑된 파일의 시작 주소를 가리킬 포인터
    char filetype[MAXLINE], buf[MAXBUF]; // 파일의 MIME 타입과 HTTP 응답을 저장할 버퍼들

    /* 1. 클라이언트에 응답 헤더 전송 */

    get_filetype(filename, filetype); // 파일의 확장자를 통해 MIME 타입을 결정 (ex. text/html, image/jpeg 등)

    // HTTP 응답 헤더 작성: 상태 코드 200 OK, 서버 정보, 연결 정보, 컨텐츠 길이, 컨텐츠 타입
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 응답 라인: HTTP 버전 1.0, 상태 코드 200 OK
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); // 응답 헤더: 서버 정보
    sprintf(buf, "%sConnection: close\r\n", buf); // 응답 헤더: 연결을 종료한다는 의미 (Connection: close)
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize); // 응답 헤더: 전송할 컨텐츠의 크기 (Content-length)
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 응답 헤더: 컨텐츠의 MIME 타입 (Content-type)

    Rio_writen(fd, buf, strlen(buf)); // 작성한 응답 헤더를 클라이언트에게 전송
    printf("Response headers:\n"); // 디버깅 목적으로 응답 헤더 출력
    printf("%s", buf); // 전송한 응답 헤더 출력

    if (strcasecmp(method, "HEAD") == 0)
        return;
    /* 2. 클라이언트에 응답 본문(body) 전송 */

    // 요청된 파일을 읽기 전용으로 열고 파일 디스크립터(srcfd)를 얻음
    srcfd = Open(filename, O_RDONLY, 0);
    char* temp=Malloc(filesize);
    Rio_readn(srcfd,temp,filesize);
    // mmap을 사용하여 파일을 메모리에 매핑: 파일 크기만큼 파일을 메모리에 읽기 전용으로 매핑함
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

    Close(srcfd); // 파일을 메모리에 매핑했으므로 파일 디스크립터를 닫아 메모리 누수를 방지

    // 메모리에 매핑된 파일 내용을 클라이언트로 전송
    Rio_writen(fd, temp, filesize);

    Free(temp);

    // mmap으로 매핑된 메모리를 해제하여 메모리 누수를 방지
    // Munmap(srcp, filesize);
}

/*
 * get_filetype - Derive file type from filename
*/

void get_filetype(char* filename, char* filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4")) // 동영상 mp4 파일 타입 추가
	    strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char* filename, char* cgiargs,char* method) {
    char buf[MAXLINE], * emptylist[] = { NULL };

    //Return first part of HTTP response
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* Child */
        //Real Server would set all CGI vars here
        setenv("QUERY_STRING", cgiargs, 1);
        setenv("REQUEST_METHOD", method, 1);
        //Redirect stdout to client
        Dup2(fd, STDOUT_FILENO);
        //Run CGI program
        Execve(filename, emptylist, environ);
    }
    //Parent waits for and reaps child
    Wait(NULL);
}