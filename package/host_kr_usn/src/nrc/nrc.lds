SECTIONS
{
	.handlers : {
		__tx_h_start = . ;
		*(nrc.txh) ;
		__tx_h_end = . ;

		__rx_h_start = . ;
		*(nrc.rxh) ;
		__rx_h_end = . ;

		__sta_h_start = . ;
		*(nrc.sta) ;
		__sta_h_end = . ; 
	}
}
