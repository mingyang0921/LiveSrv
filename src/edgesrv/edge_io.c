




	

		

 
 
#ifdef _DEBUG
printf("Recvfrom %s:%d %d bytes\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), ret);
//printOctet(stdout, pbuf, 0, ret > 200 ? 200 : ret, 2);
#endif
 


   uint64       sessid = 0;
