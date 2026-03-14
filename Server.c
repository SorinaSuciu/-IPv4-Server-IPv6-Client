// ------ SERVER IPv4 + CLIENT IPv6----
#include <sys/types.h>          
#include <sys/socket.h>         
#include <stdio.h>              
#include <string.h>     
#include <netinet/in.h>      
#include <arpa/inet.h>          
#include <unistd.h>             
#include <errno.h>              

int send_all(int sock, const char *data, int len)
{
    int total = 0;              // numarul  total de octeti trimisi pana acum
    int trimis;                 // numarul de octeti trimisi la un apel send()

    while (total < len)         // continua pana cand se trimite tot bufferul
    {
        trimis = send(sock, data + total, len - total, 0); // trimite restul

        if (trimis == -1)       // daca apare eroare
        {
            if (errno == EINTR) continue;  // reincearca daca e intrerupt de semnal
            return -1;          
        }

        total = total + trimis;        // actualizeaza numarul total trimis
    }
    return total;               // returneaza numarul total de octeti trimisi
}


int conectare_ipv6();           // functie care conecteaza clientul IPv6
int printare_ipv6(int acc);     // functie care primeste pagina si o retrimite

int soc6;                       // socketul folosit de clientul IPv6
struct sockaddr_in6 serv_addr6; // structura cu adresa serverului IPv6

// creare client IPv6 si conectare la server
int conectare_ipv6()
{
    soc6 = socket(PF_INET6, SOCK_STREAM, 0); // creeaza socket IPv6 TCP
    if (soc6 == -1) {
        perror("[IPv6] Eroare creare socket");
        return -1;
    }

    memset(&serv_addr6, 0, sizeof(serv_addr6)); // curata structura
    serv_addr6.sin6_family = PF_INET6;          

    int r = inet_pton(PF_INET6, "2620:3d:c000:178::80", &serv_addr6.sin6_addr);   // converteste adresa IPv6 in format binar

    if (r == 1) {
    }
    else if (r == 0) {// adresa invalida
        printf("[IPv6] Adresa IPv6 invalida!\n");
        close(soc6);            // inchide socketul

	return -2;
    }
    else {
        perror("[IPv6] Eroare inet_pton()");    // eroare conversie
        close(soc6);
        return -2;
    }

    serv_addr6.sin6_port = htons(80); // portul HTTP (80), convertit la big-endian(retea)

    int con;
    do {
        con = connect(soc6, (struct sockaddr*)&serv_addr6, sizeof(serv_addr6));      // incearca conectarea, handshake TCP intre client si server si pune socket ul in stare connected
    } while (con == -1 && errno == EINTR);      // se reincearca conectarea daca apare o intrerupere de semnal

    if (con == -1) {
        perror("[IPv6] Eroare connect()");      
        close(soc6);
        return -3;
    }

    printf("[IPv6] Conectare reusita.\n");
    return 0;
}


int printare_ipv6(int acc)
{
    const char *cerere_http = "GET / HTTP/1.0\r\n\r\n"; // cererea pe care o trimitem serverului IPv6
    FILE *fis = fopen("index.html", "w");
    if (!fis) {
        perror("[IPv6] Eroare creare fisier HTML");
        close(soc6);
        return -10;
    }

    if (send_all(soc6, cerere_http, strlen(cerere_http)) == -1)
    {
        perror("[IPv6] Eroare send_all()");     // eroare la trimiterea cererii
        fclose(fis);
        close(soc6);
        return -1;
    }

    char buffer[2048];          // buffer pentru datele primite
    int primit;                 // numarul de octeti primiti

    printf("[IPv6] Receptionare...\n");

    while (1)
    {	
	  do {
            primit = recv(soc6, buffer, sizeof(buffer)-1, 0); // primeste date
        } while (primit == -1 && errno == EINTR);            

        if (primit == -1 && errno == EAGAIN)
            continue;              // asteapta daca nu sunt date

        if (primit > 0)
        {
            buffer[primit] = '\0'; // termina stringul
            printf("%s", buffer);  // afiseaza pagina pe SERVER
            send_all(acc, buffer, primit);  // trimite inapoi clientului IPv4
            fwrite(buffer, 1, primit, fis);// scrie in fisier
        }
        else if (primit == 0)
        {
            printf("\n[IPv6] Serverul HTTP a inchis conexiunea.\n");
            break;                // iesire din bucla 
   	}
        else
        {
            perror("[IPv6] Eroare recv()"); // eroare primire
            fclose(fis);
            close(soc6);
            return -2;
        }
    }

    fclose(fis);                 // inchide fisierul HTML
    close(soc6);                 // inchide socket IPv6
    return 0;
}


//SERVER IPv4
int main()
{
    int soc = socket(PF_INET, SOCK_STREAM, 0);  // creeaza socket IPv4 TCP
    if (soc == -1) {
        perror("[SERVER] Eroare creare socket");
        return -1;
    }

    printf("[SERVER] Socket IPv4 creat (%d)\n", soc);

    struct sockaddr_in my_addr;                 // structura adresei serverului
    memset(&my_addr, 0, sizeof(my_addr));       // curata structura

    my_addr.sin_family = AF_INET;               // IPv4
    my_addr.sin_port = htons(22228);            // port local 22228
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);// asculta pe toate interfetele

    if (bind(soc, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {//leaga socketul la port
        perror("[SERVER] Eroare bind()");       // eroare asociere port
        close(soc);
        return -2;
    }

    if (listen(soc, 25) == -1) {                // asculta max 25 conexiuni
        perror("[SERVER] Eroare listen()");
        close(soc);
        return -3;
    }

    printf("[SERVER] Server IPv4 pornit pe portul 22228.\n");



    while (1)                                   // serverul ruleaza continuu
    {
        struct sockaddr_in client_addr;         // structura clientului
        socklen_t addr_size = sizeof(client_addr);// dimensiunea structurii clientului

       

    while(1)
     {  int acc;
     do {
         acc = accept(soc, (struct sockaddr*)&client_addr, &addr_size);
        } while (acc == -1 && errno == EINTR);

    if (acc == -1) {
    perror("[SERVER] Eroare accept()");       // eroare acceptare conexiune
    continue;                                 // trece la urmatoarea bucla
     } 

    printf("\n[SERVER] Client conectat: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); // adresa clientului IPv4, portul clientului

    while (1)
    {
    char buf[1024] = {0};                     // buffer pentru comanda clientului
    int rec = recv(acc, buf, sizeof(buf)-1, 0); // primeste comanda

    if (rec <= 0) {
        printf("[SERVER] Client deconectat.\n");
        close(acc);                           // inchide conexiunea
        break;                                // iese din bucla clientului
    }

    if (rec >= sizeof(buf)-1) {               // daca comanda e prea lunga
        char err[] = "Comanda prea lunga.\n";
        send_all(acc, err, strlen(err));
        continue;                             // conexiunea ramane deschisa
    }

    buf[rec] = '\0';                          // termina stringul
    printf("[SERVER] Comanda primita: '%s'\n", buf);

    if (strcmp(buf, "DISC#") == 0) {
        printf("[SERVER] Client a cerut disconnect.\n");
        close(acc);
        break;                                // inchide doar acest client
    }

    if (strcmp(buf, "04#") == 0)              // verifica comanda daca este 04
    {
        printf("[SERVER] Comanda 04 primita Ã¢â â Pornesc clientul IPv6.\n");

        int err = conectare_ipv6();           // porneste clientul IPv6
        if (err < 0)
        {
            char rsp[128];
            sprintf(rsp, "Eroare la conectarea IPv6. Cod=%d\n", err);
            send_all(acc, rsp, strlen(rsp));
            continue;                         // conexiunea ramane deschisa
        }

        err = printare_ipv6(acc);             // primeste pagina si o retrimite
        if (err < 0)
        {
            char rsp[128];
            sprintf(rsp, "Eroare la receptionarea IPv6. Cod=%d\n", err);
            send_all(acc, rsp, strlen(rsp));
            continue;                         // conexiunea ramane deschisa
        }

        char ok[] = "\nClient IPv6 executat cu succes (cmd 04).\n";
        send_all(acc, ok, strlen(ok));        // confirmare finala
        continue;                              // conexiunea ramane deschisa
    }

    char rsp[] = "Comanda neimplementata.\n";
    send_all(acc, rsp, strlen(rsp));          // trimite mesaj default
  }       
  }
    close(soc);                                  // inchide socketul serverului
    return 0;                                    // sfarsit program
}
}
