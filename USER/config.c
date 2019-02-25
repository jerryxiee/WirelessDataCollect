#include "config.h"
#include "stmflash.h"
#include "usart.h"
#include "string.h"
#include "crc.h"
#include "typetrans.h"
#include "userwifi.h"
#include "core_cm4.h"
#include <stdlib.h>
#include <stdio.h>

u8 RSI_JOIN_SSID[RSI_JOIN_SSID_MAX_LENGTH] = "418_L";
u8 RSI_PSK[RSI_PSK_MAX_LENGTH] = "518518518"; 

u8 catPara[PARA_CAT_CH_MAX_LENGTH]={0};//存储连接后的数据


//**********************
//函数作用：下载系统运行所需的参数
//数据读取格式：\r\n RSI_JOIN_SSID \r\n
//
//**********************
//从flash中提取出数据长度，包括长度、数据、分隔符、crc、结束符
u32 getCatParaChLenInFlash(u32 startAddr){
	u32 catParaChLen;
	if(FLASH_HEAD_LENGTH_BYTES==4){//32bits保存
		catParaChLen = (u32)STMFLASH_ReadWord(startAddr);
	}else if(FLASH_HEAD_LENGTH_BYTES==2){  //16bits保存
		catParaChLen = (u32)STMFLASH_ReadHalfWord(startAddr);
	}else if(FLASH_HEAD_LENGTH_BYTES==1){  //8bits保存
		catParaChLen = (u32)STMFLASH_ReadByte(startAddr);
	}
	return catParaChLen;
}
//下载一个扇区的数据，包括校验
u8 loadParaAndCheck(u8 * catPara,u32 startAddr){
	#if PRINT_UART_LOG
	printf("Loading Paras...\r\n");
	#endif
	//先读出长度
	u32 catParaChLen = getCatParaChLenInFlash(startAddr);
	#if PRINT_UART_LOG
	printf("Para Array Length : %d\r\n",catParaChLen);
	#endif
	if(catParaChLen>=PARA_CAT_CH_MAX_LENGTH){
		return LOAD_PARA_TOO_LONG_ERROR;
	}
//	catPara = (u8 * )calloc(1,(catParaChLen+1)*sizeof(u8));  //多一位，该段空间全部为0
//	catPara = (u8 * )malloc((catParaChLen+1)*sizeof(u8));  //多一位，该段空间全部为0
//	if(catPara == NULL){//没有分配成功
//		printf("Load Para Calloc Error!\r\n");
//		free(catPara);//释放
//		return LOAD_PARA_SPACE_ALLO_ERROR;
//	}
	memset(catPara,0,PARA_CAT_CH_MAX_LENGTH);//全部赋值0
	STMFLASH_ReadBytes(startAddr,catPara,catParaChLen);//读取main区中所有数据
	#if PRINT_UART_LOG
	printf("Checking CRC...\r\n");
	#endif
	CRC_TYPE crc = CalCrc(0, (c8 *)catPara,catParaChLen- sizeof(FLASH_LABLE_TYPE) - FLASH_CRC_BYTE_LENGTH,0x8005);
	//计算得到crc转化为字符串，长度固定，高位补0
	u8 crcStr[FLASH_CRC_BYTE_LENGTH+1];memset(crcStr,'0',FLASH_CRC_BYTE_LENGTH);crcStr[FLASH_CRC_BYTE_LENGTH]=0;//初始化"00000\0"
	u8 * crcStrItoa = itoa(crc);
	memcpy((char *)crcStr,crcStrItoa,strlen((c8*)crcStrItoa));//转化为字符串
	//获取在flash中的crc字符串
	u8 crcStrFlash[FLASH_CRC_BYTE_LENGTH+1]={0};memcpy(crcStrFlash,catPara+catParaChLen-sizeof(FLASH_LABLE_TYPE)-FLASH_CRC_BYTE_LENGTH,FLASH_CRC_BYTE_LENGTH);
	#if PRINT_UART_LOG
	printf("CRC Computed : %s \r\nCRC in Flash : %s\r\n",crcStr,crcStrFlash);
	#endif
	if(strcmp((c8*)crcStr,(c8*)crcStrFlash) != 0){//对比
		#if PRINT_UART_LOG
		printf("CRC Check Error!\r\n");
		#endif
		return LOAD_PARA_CRC_ERROR;
	}else{
		#if PRINT_UART_LOG
		printf("CRC Check Correct!\r\n");
		#endif
	}
//	if(CalCrc(0, (c8 *)catPara,catParaChLen- sizeof(u8),0x8005)!=0){//得到尾部加入crc后的crc，如果正确一定等于0
//		printf("CRC Check Error!\r\n");
////		free(catPara);//释放
//		return LOAD_PARA_CRC_ERROR;
//	}
	#if PRINT_UART_LOG
	printf("Paras Unzipping...\r\n");
	#endif
	//分割数据
	u32 paraStartAddr =(u32) (catPara + FLASH_HEAD_LENGTH_BYTES);//数据存储开始的位置，在数组中的下标
	/**SSID*/
	u32 splitAddr = (u32) strchr((c8 *)(paraStartAddr),FLASH_LABEL_SPLIT);//得到分隔符的地址
	memcpy((u8 *)RSI_JOIN_SSID,(u8 *)paraStartAddr,splitAddr - paraStartAddr);
	paraStartAddr = splitAddr+1;//开始下一个数据
	#if PRINT_UART_LOG
	printf("RSI_JOIN_SSID : %s\r\n",RSI_JOIN_SSID);
	#endif
	/*PSK*/
	splitAddr = (u32) strchr((c8 *)(paraStartAddr),FLASH_LABEL_SPLIT);//得到分隔符的地址
	memcpy((u8 *)RSI_PSK,(u8 *)paraStartAddr,splitAddr - paraStartAddr);
	paraStartAddr = splitAddr+1;//开始下一个数据
	#if PRINT_UART_LOG
	printf("RSI_PSK : %s\r\n",RSI_PSK);
	#endif
//	free(catPara);//释放
	return LOAD_PARA_SUCCESS;
}

//**********************
//函数作用：保存参数
//
//**********************

////写入word（4字节）数据，改变写地址
u8 writeFlashWordsIncAddr(u32 * pBuff,u32 * WriteAddr,u16 WordLen){
	for(int i =0;i<WordLen;i+=1){
		if(FLASH_ProgramByte(*WriteAddr,*(pBuff + i)) != FLASH_COMPLETE)//写入数据
		{ 
			return WRITE_PARA_FAILED;	//写入异常
		}
		*WriteAddr += 4;//4byte为单位
	}
	return WRITE_PARA_SUCCESS;
}
//写入byte数据，同时改变写地址
u8 writeFlashByteIncAddr(u8 * pBuff,u32 * WriteAddr,u16 u8Len){
	for(int i =0;i<u8Len;i+=1){
		if(FLASH_ProgramByte(*WriteAddr,*(pBuff + i))!=FLASH_COMPLETE)//写入数据
		{ 
			return WRITE_PARA_FAILED;	//写入异常
		}
		*WriteAddr += 1;//1byte为单位
	}
	return WRITE_PARA_SUCCESS;
}

////写入word（4字节）数据，不改变写地址
u8 writeFlashWords(u32 * pBuff,u32 WriteAddr,u16 WordLen){
	u32 writeAddrNow = WriteAddr;
	for(int i =0;i<WordLen;i+=1){
		if(FLASH_ProgramByte(writeAddrNow,*(pBuff + i)) != FLASH_COMPLETE)//写入数据
		{ 
			return WRITE_PARA_FAILED;	//写入异常
		}
		writeAddrNow += 4;//4byte为单位
	}
	return WRITE_PARA_SUCCESS;
}
//写入byte数据，同时不改变写地址
u8 writeFlashByte(u8 * pBuff,u32 WriteAddr,u16 u8Len){
	u32 writeAddrNow = WriteAddr;
	for(int i =0;i<u8Len;i+=1){
		if(FLASH_ProgramByte(writeAddrNow,*(pBuff + i))!=FLASH_COMPLETE)//写入数据
		{
			return WRITE_PARA_FAILED;	//写入异常
		}
		writeAddrNow += 1;//1byte为单位
	}
	return WRITE_PARA_SUCCESS;
}
////********************
////下面是保存数据的位置顺序
////********************
////0、保存数据个数的信息，单位是word（4bytes）
//u8 saveParaByteLenIncAddr(u32 * LastWriteAddr){
//	u32 WriteAddr = FLASH_SAVE_ADDR_MAIN;
//	u32 ParaByteLen = *LastWriteAddr - FLASH_SAVE_ADDR_MAIN;
//	return writeFlashWordsIncAddr(&ParaByteLen,& WriteAddr,1);
//}

////1、RSI_JOIN_SSID
//u8 saveJoinSSID(u32 * WriteAddr){
//	u16 len = strlen(RSI_JOIN_SSID);
//	return writeFlashByteIncAddr((u8 *)RSI_JOIN_SSID,WriteAddr,len);
//}
////2、RSI_PSK
//u8 savePSK(u32 * WriteAddr){
//	u16 len = strlen(RSI_PSK);
//	return writeFlashByteIncAddr((u8 *)RSI_PSK,WriteAddr,len);
//}

//u8 writePara(void){
//	u32 WriteByte = FLASH_SAVE_ADDR_MAIN + FLASH_HEAD_LENGTH_BYTES;//从这里开始写数据
//	FLASH_Unlock();//解锁 
//	FLASH_DataCacheCmd(DISABLE);//FLASH擦除期间,必须禁止数据缓存
//	FLASH_Status status = status=FLASH_EraseSector(FLASH_SAVE_SECTOR_BACKUP,VoltageRange_3);//擦除掉变量主存储区的所有数据
//	if(status != FLASH_COMPLETE){//未成功擦除数据，保存失败
//		return WRITE_PARA_FAILED;
//	}else{//擦除成功
//		//存储JoinSSID，把所有所有数据的长度位数先空出来		
//		if(saveJoinSSID(&WriteByte) != FLASH_COMPLETE ){
//			return WRITE_PARA_FAILED;
//		}
//		//存储PSK
//		if(savePSK(&WriteByte) != FLASH_COMPLETE ){
//			return WRITE_PARA_FAILED;
//		}
//		if(saveParaByteLenIncAddr(&WriteByte) != FLASH_COMPLETE){
//			return WRITE_PARA_FAILED;
//		}
//	}
//	FLASH_DataCacheCmd(ENABLE);	//FLASH擦除结束,开启数据缓存
//	FLASH_Lock();//上锁	
//	return 0;
//}

//获取保存所有数据所需的空间
//包括数据长度、分隔符、数据、结束符
u32 getParaLen(){
	u16 catChLen = 0;
	catChLen += FLASH_HEAD_LENGTH_BYTES;//用于保存数据长度的byte数
	catChLen += strlen((c8*)RSI_JOIN_SSID);
	catChLen += sizeof(FLASH_LABLE_TYPE); //分隔符占用1byte
	catChLen += strlen((c8*)RSI_PSK);
	catChLen += sizeof(FLASH_LABLE_TYPE); //分隔符占用1byte
	catChLen += FLASH_CRC_BYTE_LENGTH;//加入一个CRC校验

	catChLen += sizeof(FLASH_LABLE_TYPE); //结束符号占用1byte
//	printf("strlen(RSI_JOIN_SSID) : %d\r\n",strlen((c8*)RSI_JOIN_SSID));
//	printf("FLASH_LABEL_SPLIT : %d\r\n",sizeof(FLASH_LABLE_TYPE));
//	printf("strlen(RSI_PSK) : %d\r\n",strlen((c8*)RSI_PSK));
//	printf("CRC_TYPE : %d\r\n",FLASH_CRC_BYTE_LENGTH);
//	printf("FLASH_LABEL_END : %d\r\n",sizeof(FLASH_LABLE_TYPE));
	return 	catChLen;
}

//将所有要保存的数据都连接起来,中间用分隔符分开，返回数据的长度，不包括最后的'\0'
//分配失败的话会返回0
u32 getCatPara(void){
	u32 catParaLen = getParaLen();
	#if PRINT_UART_LOG
	printf("catParaLen = %d\r\n",catParaLen);
	#endif
//	free(catPara);//每次都要释放掉以前的catCh内存
//	catPara = (u8 *)calloc(1,(catParaLen+1)*sizeof(u8));//多了一位，用于最后放结束符号'\0'
//	if(catPara == NULL){//没有分配成功
//		free(catPara);
//		return 0;
//	}	
	u32 i=0;
	for(;i<FLASH_HEAD_LENGTH_BYTES;i++){//存chLen
		*(catPara+i) = (u8)(catParaLen>>(8*i));
	}
	memcpy(catPara+i,RSI_JOIN_SSID,strlen((c8*)RSI_JOIN_SSID));i += strlen((c8*)RSI_JOIN_SSID);//存ssid
	*(catPara+i) = FLASH_LABEL_SPLIT;i += sizeof(FLASH_LABLE_TYPE);//分隔符1byte
	memcpy(catPara+i,RSI_PSK,strlen((c8*)RSI_PSK));i += strlen((c8*)RSI_PSK);//PSK
	*(catPara+i) = FLASH_LABEL_SPLIT;i += sizeof(FLASH_LABLE_TYPE);//分隔符1byte
	CRC_TYPE crc = CalCrc(0, (c8 *)catPara,catParaLen- FLASH_CRC_BYTE_LENGTH - sizeof(FLASH_LABLE_TYPE),0x8005);//得到crc,把结束符号和crc位都去掉
	#if PRINT_UART_LOG
	printf("GET CRC VAL : %d\r\n",crc);
	#endif
	u8 * crcStr = itoa(crc);
	u8 crcStrKeepZero[FLASH_CRC_BYTE_LENGTH+1] = {0};
	memset(crcStrKeepZero,'0',FLASH_CRC_BYTE_LENGTH);//'0'...'0''\0'
	memcpy(crcStrKeepZero,crcStr,strlen((c8 *)crcStr));//不会擅自在后面加'\0'
	memcpy(catPara+i,crcStrKeepZero,strlen((c8*)crcStrKeepZero));i += strlen((c8*)crcStrKeepZero);//crcStr
	#if PRINT_UART_LOG
	printf("GET CRC STR VAL : %s\r\n",crcStrKeepZero);
	#endif
//	for(u16 i_this;i_this<sizeof(CRC_TYPE);i++,i_this++){//存crc16
//		*(catPara+i) = (u8)(crc>>(8*i_this));
//	}
	*(catPara+i) = FLASH_LABEL_END;i += sizeof(FLASH_LABLE_TYPE);//结尾
	*(catPara+i) = '\0';//不需要也可以，calloc初始化的时候已经全部初始化为0
	#if PRINT_UART_LOG
	printf("catPara : %s len : %d\r\n",catPara+FLASH_HEAD_LENGTH_BYTES,strlen((c8*)(catPara+FLASH_HEAD_LENGTH_BYTES)));
	#endif
	
	if(catParaLen-FLASH_HEAD_LENGTH_BYTES == strlen((c8 *) (catPara + FLASH_HEAD_LENGTH_BYTES))){
	#if PRINT_UART_LOG
		printf("LEN IS CORRECT!\r\n");
	#endif
	}else{
	#if PRINT_UART_LOG
		printf("LEN %d(Real : %d) IS NOT CORRECT!\r\n",strlen((c8 *) (catPara + FLASH_HEAD_LENGTH_BYTES)),catParaLen-FLASH_HEAD_LENGTH_BYTES);
	#endif
	}
	return catParaLen;
}

//写一个扇区的数据
void writeSectorPara(void){
	u32 catParaLen = getCatPara();
//	if(catParaLen){//如果长度是0，说明保存数据的数组分配失败了。不写，直接返回	
//		printf("\r\nCALLOC Failed!\r\nStop Writing Flash!\r\n");	
//		return;
//	}
	FLASH_Status status =FLASH_EraseSector(FLASH_SAVE_SECTOR_MAIN,VoltageRange_3);//擦除掉变量主存储区的所有数据
	if(status != FLASH_COMPLETE){//未成功擦除数据，保存失败
	#if PRINT_UART_LOG
		printf("\r\nMain flash Erased Unsuccessfully!\r\n");
	#endif
	}else{
	#if PRINT_UART_LOG
		printf("\r\nMain flash Erased Successfully!\r\n");
	#endif
	}
	if(writeFlashByte(catPara,FLASH_SAVE_ADDR_MAIN,catParaLen) == WRITE_PARA_FAILED){ //失败
	#if PRINT_UART_LOG
		printf("\r\nMain flash Writen Unsuccessfully!\r\n");
	#endif
	}else{
	#if PRINT_UART_LOG
		printf("\r\nMain flash Writen Successfully!\r\n\r\n");
	#endif
	}
	status = FLASH_EraseSector(FLASH_SAVE_SECTOR_BACKUP,VoltageRange_3);//擦除掉变量主存储区的所有数据
	if(status != FLASH_COMPLETE){//未成功擦除数据，保存失败
	#if PRINT_UART_LOG
		printf("\r\nBackup flash Erased Unsuccessfully!\r\n");
	#endif
	}else{
	#if PRINT_UART_LOG
		printf("\r\nBackup flash Erased Successfully!\r\n");
	#endif
	}
	if(writeFlashByte(catPara,FLASH_SAVE_ADDR_BACKUP,catParaLen) == WRITE_PARA_FAILED){
	#if PRINT_UART_LOG
		printf("\r\nBackup flash Writen Unsuccessfully!\r\n");
	#endif
	}else{
	#if PRINT_UART_LOG
		printf("\r\nBackup flash Writen Successfully!\r\n");
	#endif
	}
}





//UART指令处理

//********************
//函数作用：命令拆分
//  ' '：分割指令和设置值
//  "\r\n"一条指令终止
//********************

void putZero(u8 * pChar,u16 i){
	*(pChar+i) = 0;
}

//获取splitCmd函数需要返回的状态
u8 getSplitCmdFunReturn(u8* pCmd,u8 * pValue){
	if(*pValue != 0  &&  *pCmd != 0){//cmd和val都不是空的
		return CMD_VALUE_SPLIT_OK;
	}else if(*pCmd != 0){ //如果cmd不是空的
		return CMD_SPLIT_OK;
	}else{//两个都是空的返回error，表示没有指令或者分割指令失败
		return CMD_VALUE_SPLIT_ERROR;
	}
}


u8 splitCmd(volatile CMD_QUEUE * pQueue,u8 * pCmd,u8 * pValue)
{
	u16 i_cmd=0,i_val=0;
	u8 SplitFlag = CMD_VALUE_NOT_SPLIT_STA;
	u8 uart_data;
	u16 length = uart_queue_length(pQueue); //得到长度
	for(u16 forNum = 0;pQueue->CmdCompleteFlag==CMD_COMPLETED;)//一次cmd完成了
	{
		if(forNum >= length){//超过了数据个数
			return CMD_VALUE_SPLIT_ERROR;
		}
		uart_data = uart_queue_pop(pQueue);
		forNum++;
		
		if(uart_data == CMD_VALUE_SPLIT_CHAR && SplitFlag != CMD_VALUE_SPLITED_STA)//分隔符来了而且只能来一次也就是支持输入"SET    111"这样的数据，不会把后面几个空格当做分隔符
		{
			SplitFlag = CMD_VALUE_SPLITED_STA;
		}
		else //不是分隔符
		{
			if(uart_data != '\r')
			{
				if(uart_data == '\n') //前面没有'\r'直接出现了\n
				{
					uart_queue_clear(pQueue);
					putZero(pValue,i_val);
					putZero(pCmd,i_cmd);
					
					return CMD_VALUE_SPLIT_ERROR;
				}		
				if(SplitFlag == CMD_VALUE_NOT_SPLIT_STA) //还没有到分隔符，也就是还在获取cmd
				{
					*(pCmd+i_cmd) = uart_data;
					i_cmd++;
				}
				else
				{
					*(pValue+i_val) = uart_data;
					i_val++;
				}

			}
			else if(uart_data == '\r' && uart_queue_pop(pQueue) == '\n')  //\r\n   结束符号
			{
				uart_queue_clear(pQueue);
				putZero(pValue,i_val);
				putZero(pCmd,i_cmd);
				return getSplitCmdFunReturn(pCmd,pValue);//正常停止时，需要判断是cmd还是cmd+val
			}	
			else if(uart_data == '\r' && uart_queue_pop(pQueue) != '\n') //出现了"\r" "非\n"的情况
			{
				uart_queue_clear(pQueue);
				putZero(pValue,i_val);
				putZero(pCmd,i_cmd);
				return CMD_VALUE_SPLIT_ERROR;
			}
		}

	}
	
	uart_queue_clear(pQueue);
	putZero(pValue,i_val);
	putZero(pCmd,i_cmd);
	return NONE_CMD_VALUE_MSG;
	
}


////////////处理CMD VALUE格式///////////////


//*************************
//处理Cmd
//*************************
//系统重启
static void SystemReset(void){
	__DSB();                                                     /* Ensure all outstanding memory accesses included
															  buffered write are completed before reset */
	SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)      |
				(SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
				SCB_AIRCR_SYSRESETREQ_Msk);                   /* Keep priority group unchanged */
	__DSB();                                                     /* Ensure completion of memory access */
	while(1);  
}

//输出帮助文件
void getHelp(void){
#if PRINT_UART_LOG
	printf("\r\n ============  Help Document  ============ \r\n\r\n");
	//CMD + VAL
	printf("- SET_RSI_JOIN_SSID    : Set RSI_JOIN_SSID\r\n  EG. SET_RSI_JOIN_SSID SORL_WIFI\r\n\r\n");
	printf("- SET_RSI_PSK          : Set RSI_PSK\r\n  EG. SET_RSI_PSK 123456\r\n\r\n");
	//CMD
	printf("- HELP                 : Print Help Document\r\n  EG. HELP\r\n\r\n");
	printf("- SAVE_ALL_PARA        : Save All Parameters in Flash\r\n  EG. SAVE_ALL_PARA\r\n\r\n");
	printf("- RESET_SYSTEM         : Reboot MCU\r\n  EG. RESET_SYSTEM\r\n\r\n");
#endif
}
void handleCmd(c8 * cmd){
#if PRINT_UART_LOG
	printf("Received CMD:\r\n  %s\r\n",cmd);
#endif
	if(strcmp(cmd,CMD_HELP) == 0){//帮助文件
		getHelp();
	}else if(strcmp(cmd,CMD_SAVE_ALL_PARA) == 0){//保存所有数据到flash
#if PRINT_UART_LOG
		printf("Saving All Paras\r\n");
#endif
		FLASH_Unlock();//解锁 
		FLASH_DataCacheCmd(DISABLE);//FLASH擦除期间,必须禁止数据缓存
		writeSectorPara();
		FLASH_DataCacheCmd(ENABLE);	//FLASH擦除结束,开启数据缓存
		FLASH_Lock();//上锁
#if PRINT_UART_LOG
		printf("Saving Process End\r\n");
#endif
	}else if(strcmp(cmd,CMD_RESET_SYSTEM) == 0){//重新启动wifi
#if PRINT_UART_LOG
		printf("Reset...\r\n");
#endif
        SystemReset();                                      /* wait until reset */
	}
}
//*************************
//处理Cmd Val
//*************************
//设置string类型的数据
//obj：对象
//val：数值
void setStrVal(c8 * val,c8 * obj,c8 * objName){//设置string value
#if PRINT_UART_LOG	
	printf("Setting %s...\r\n",objName);
#endif
	memcpy((u8 *)obj,val,strlen(val));
	memset((u8 *)(obj+strlen(val)),'\0',1);//最后面赋值0
#if PRINT_UART_LOG
	printf("Now %s = \"%s\"\r\n",objName,obj);
	printf("Setted %s OK\r\n",objName);
	printf("To Save Flash.Send \"%s\"\r\n",CMD_SAVE_ALL_PARA);	
#endif
}

//总的cmd+Val命令处理
void handleCmdVal(c8 * cmd,c8 * val){
#if PRINT_UART_LOG
	printf("RECEIVED\r\n  CMD : %s \r\n  VAL : %s\r\n",cmd,val);
#endif
	if(!strcmp(cmd,CMD_SET_JOIN_SSID)){
		setStrVal(val,(c8*)RSI_JOIN_SSID,RSI_JOIN_SSID_STRNAME);
	}else if(!strcmp(cmd,CMD_SET_PSK)){
		setStrVal(val,(c8*)RSI_PSK,RSI_PSK_STRNAME);
	}
	

}


//*************************
//处理分离错误问题
//*************************
u8 handleSplitError(){
	//TODO
}


u8 dealCmdMsg(volatile CMD_QUEUE * pQueue){
	u8 cmd[USART_REC_CMD_LEN];
	u8 val[USART_REC_VAL_LEN];
	u8 SplitSta = splitCmd(&CMD_RX_BUF,cmd,val);
	switch(SplitSta){
		case CMD_VALUE_SPLIT_OK:   //cmd   value都有
			handleCmdVal((c8 *)cmd,(c8 *)val);
			break;
		case CMD_SPLIT_OK:  //直接受到了cmd
			handleCmd((c8 *)cmd);
			break;
		case CMD_VALUE_SPLIT_ERROR: //数据split出错
			handleSplitError();
		#if PRINT_UART_LOG
			printf("Received CMD ERROR\r\n");
		#endif
			break;
		case NONE_CMD_VALUE_MSG:   //没有接收到数据
//			printf("None CMD\r\n");
			break;
		default:
			
			break;
	}
}





