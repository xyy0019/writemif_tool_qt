#include <iostream>
#include <string.h>
using namespace std;

#define SHOWPROCESS 0

void int_to_binary_string(unsigned int num, char *binary_str) {
    binary_str[32] = '\0';
    for (int i = 31; i >= 0; i--) {
        binary_str[i] = (num & 1) ? '1' : '0';
        num >>= 1;
    }
}

char* strxor(char *result, const char *s1, const char *s2)
{
    int i=0;
    while(s1[i] && s2[i]){
        if(s1[i] != s2[i]){
            result[i] = '1';
        }
        else{
            result[i] = '0';
        }
        ++i;
    }
    result[i] = '\0';
    return result;
}


char* strlmv(char *s)
{
    int i=0;
    while(s[i]){
        s[i] = s[i+1];
        ++i;
    }
    return s;
}


void repeat(char c, int n)
{
    while(n--){
        cout<<c;
    }
}

void strm2div(const char *strM, const char *strP, char *strQ, char *strR)
{
    int lM = strlen(strM);
    int lP = strlen(strP);
    int L = lM+lP;
    int i;

    char *sM = new char[L+1];

    for(i=0; i<L; ++i){
        if(i<lM){
            sM[i] = strM[i];
        }
        else{
            sM[i] = '0';
        }
    }
    sM[i] = '\0';

    strncpy(strR, sM, lP);
    strR[lP] = '\0';

    #if (SHOWPROCESS==1)
        cout<<strM<<'/'<<strP<<"的计算过程:"<<endl;
        cout<<sM<<endl;
    #endif

    for(i=0; i<lM; ++i){
        #if (SHOWPROCESS==1)
            if(i!=0){
                repeat(' ', i);
                cout<<strR<<endl;
            }
        #endif

        if(strR[0]=='1'){
            #if (SHOWPROCESS==1)
                repeat(' ', i);
                cout<<strP<<endl;
            #endif

            strxor(strR, strR, strP);
            strQ[i] = '1';
        }
        else{
            #if (SHOWPROCESS==1)
                repeat(' ', i);
                repeat('0', lP);
                cout<<endl;
            #endif

            strQ[i] = '0';
        }
        strlmv(strR);
        strR[lP-1] = sM[lP+i];
        strR[lP] = '\0';
    }
    strR[lP-1] = '\0';
    strQ[i] = '\0';

    #if (SHOWPROCESS==1)
        repeat(' ', i);
        cout<<strR<<endl;
    #endif

    delete sM;
}

char *crc16(const char *strM, char *fcs)
{
    char *tmQ = new char[strlen(strM)+1];
    strm2div(strM, "11000000000000101", tmQ, fcs);
    return fcs;
}


void div()
{
    char Q[1000], R[1000];
    char M[1000], P[1000];
    for(;;){
        cout<<"输入被除数(二进制):";
        cin>>M;
        if(M[0]!='0' || M[1]!='\0'){
            cout<<"输入除数(二进制):";
            cin>>P;
            if(P[0]!='0' || P[1]!='\0'){
                strm2div(M, P, Q, R);
                cout<<M<<'/'<<P<<endl;
                cout<<"\t商为:"<<Q<<endl;
                cout<<"\t余数:"<<R<<endl<<endl;
            }
            else{
                break;
            }
        }
        else{
            break;
        }
    }
}

//crc16校验演示
void crc()
{
    char inbuf[1000], fcs[1000];
    cout<<"输入M:";
    cin>>inbuf;
    while(inbuf[0]!='0' || inbuf[1]!='\0'){
        cout<<"M:"<<inbuf<<endl;
        cout<<"FCS:"<<crc16(inbuf, fcs)<<endl;
        cout<<"校验帧:"<<inbuf<<fcs<<endl<<endl;
        cout<<"输入M:";
        cin>>inbuf;
    }
}
