/******************************************************************************/
/*                                                                            */
/*                  Action Replay Backup Helper Ver.0.02                      */
/*                                                                            */
/*                                               Nekokabu                     */
/*                                                                            */
/*                                       HP:    http://nekokabu.s7.xrea.com/  */
/*                                       Mail:  nekokabu@gmail.com            */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_WIIDUMP_BIN "%s\\wiidump.bin"

#define UNICODE 0

/*===================================*/
/*  プログラムの使い方               */
/*===================================*/
void usage(void){
	printf("usage: arbh {option} {driveletter/filename}...\n\n");
	printf("ex:\n");
	printf(" Patch a SD card:\n");
	printf("    arbh -p F:\n");			//指定したSDカードにパッチ
	printf(" Read AR data from SD card:\n");
	printf("    arbh -s F: OUTPUTFILE.bin\n");	//指定したSDカードからデータの保存
	printf(" Generate a AR gcm image from data files:\n");
	printf("    arbh -gcm INPUTFILE1.bin INPUTFILE2.bin OUTPUTFILE.gcm\n");	//データからARのGCMファイルを生成
	
	getchar();
}

/*========================================================*/
/*  SDカードがDVDDUMPでフォーマットされているかチェック   */
/*  戻り値 エラーは0                                      */
/*========================================================*/
int chkSdCard(char* driveletter){
	FILE *fp;
	char rBuffer[20] = "";
	unsigned int size = 0;
	
	printf("Checking SDcard... ");
	if( (strlen(driveletter) == 2) && (driveletter[1] == ':') ){

		////////////////////////////////////
		// wiidump.binを開く
		sprintf(rBuffer, FILE_WIIDUMP_BIN, driveletter);
		if( (fp = fopen(rBuffer, "rb")) == NULL ){
			printf("Cannot open a %s\n", rBuffer);
			return 0;
		}

		////////////////////////////////////
		// wiidump.binの先頭16バイトを読み込み
		fseek(fp, 0x0L, SEEK_SET);
		fread(rBuffer, sizeof(char), 16, fp);

		////////////////////////////////////
		// wiidump.binのファイルサイズの取得
		fseek(fp, 0x0L, SEEK_END);
		size = ftell(fp);
		fclose(fp);
		
		////////////////////////////////////
		// "HELLODVDDUMP =]"かどうかチェック
		if( strcmp(rBuffer, "HELLODVDDUMP =]") == 0 ){
			if(size >= 40*1024*1024){
				printf("OK.\n");
				return 1;
			}
			printf("SDcard is too small!\n");
		}
		else printf("SDcard is not formatted for DVDDUMP\n");

	}
	else printf("Wrong Drive Letter: %s\n", driveletter);
	
	return 0;
}

/*===================================*/
/*  SDカードのChunksizeにパッチ      */
/*  戻り値 エラーは0                 */
/*===================================*/
int patchSdCard(char* driveletter){
	FILE *fp;
	char rBuffer[20] = "";

	////////////////////////////////////
	// wiidump.binを開く
	sprintf(rBuffer, FILE_WIIDUMP_BIN, driveletter);
	if( (fp = fopen(rBuffer, "rb+")) == NULL ){
		printf("Cannot open a %s\n", rBuffer);
		return 0;
	}
	
	////////////////////////////////////
	// 0x00000040(Chunksize)を0x50000000に変更
	rBuffer[0] = 0x50;
	rBuffer[1] = 0x00;
	rBuffer[2] = 0x00;
	rBuffer[3] = 0x00;
	fseek(fp, 0x40L, SEEK_SET);
	fwrite(rBuffer, sizeof(char), 4, fp);
	printf("Patch Chunksize to 0x50000000.\n");
	
	fclose(fp);
	return 1;
}


/*===================================*/
/*  ARのデータをファイルに保存       */
/*  戻り値 エラーは0                 */
/*===================================*/
int ripArData(char* driveletter, char* file){
	FILE *fp, *fpt;
	char gameName[0x30] = "";
	unsigned char rBuffer[1024];

	////////////////////////////////////
	// wiidump.binを開く
	sprintf(gameName, FILE_WIIDUMP_BIN, driveletter);
	if( (fp = fopen(gameName, "rb")) == NULL ){
		printf("Cannot open a %s\n", gameName);
		return 0;
	}

	////////////////////////////////////
	// 保存先のファイルを開く
	if( (fpt = fopen(file, "wb")) == NULL ){
		printf("Cannot open a %s\n", file);
		return 0;
	}
	
	////////////////////////////////////
	// ゲーム名の取得
	fseek(fp, 0x10L, SEEK_SET);
	fread(gameName, sizeof(char), 0x30, fp);
	if( strcmp(gameName, "NoGame") == 0){
		printf("There are not dumped data in SDcard!\n");
		return 0;
	}
	printf("Disc Title: %s\n", gameName);
	
	//////////////////////////////////////////////////
	////////////////////////////////////
	// wiidump.binのチェック。
	//  パッチがあたってるか
	fseek(fp, 0x40L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x50 ) || ( rBuffer[1] != 0x00 ) || ( rBuffer[2] != 0x00 ) || ( rBuffer[3] != 0x00 ) ){
		printf("SDcard does not apply a patch.\n Please use \"-p\" option.\n");
		return 0;
	}

	////////////////////////////////////
	// wiidump.binのチェック。
	//  ディスクサイズ(LBA)のチェック。
	fseek(fp, 0x48L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x00 ) || ( rBuffer[1] != 0x0A ) || ( rBuffer[2] != 0xE0 ) || ( rBuffer[3] != 0xB0 ) ){
		printf("SDcard data is not dumped from GC disc.\n");
		return 0;
	}

	////////////////////////////////////
	// wiidump.binのチェック。
	//  吸出しに成功したかフラグのチェック
	//  ※DVDDUMPでエラーが出ずに正常にダンプができていれば0x00000053が1になる。
	//    AR吸い出す場合はここが必ず0になる
	fseek(fp, 0x50L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x00 ) || ( rBuffer[1] != 0x00 ) || ( rBuffer[2] != 0x00 ) || ( rBuffer[3] != 0x00 ) ){
		printf("SDcard data is not dumped from AR disc.\n");
		return 0;
	}

	//////////////////////////////////////////////////
	////////////////////////////////////
	// ダンプされたデータをファイルに保存
	fseek(fp, 0x200L, SEEK_SET);
	fseek(fpt, 0x0L, SEEK_SET);
	
	while(1){
		if( fread(rBuffer, sizeof(unsigned char), sizeof(rBuffer), fp) < sizeof(rBuffer) ){
			printf("Read Error: %.8X\n", ftell(fp));
			break;
		}
		
		////////////////////////////////////
		// エラーセクタ(すべてのデータが0xA8)の検出
		if( rBuffer[0] == 0xA8 ){
			int i;
			for(i = 0; i < sizeof(rBuffer); i++){
				if( rBuffer[i] != 0xA8 ) break;
			}
			if( i == sizeof(rBuffer) ){
				printf("Dumped Data Size: %d bytes\n", ftell(fpt));
				
				fclose(fp);
				fclose(fpt);

				return 1;
			}
		}
		
		if( fwrite(rBuffer, sizeof(unsigned char), sizeof(rBuffer), fpt) < sizeof(rBuffer) ){
			printf("Write Error: %.8X\n", ftell(fpt));
			break;
		}
	}
	fclose(fp);
	fclose(fpt);
	
	return 0;
}

/*==============================================*/
/*  dstの指定したアドレスに、srcのデータを追加  */
/*  戻り値 エラーは1                            */
/*==============================================*/
int addData(FILE *dst, FILE *src, long offset){

	unsigned char rBuffer[1024];
	int tmp = 0;

	if (dst == NULL) return 1;
	if (src == NULL) return 1;

	fseek(dst, offset, SEEK_SET);
	fseek(src, 0x0L, SEEK_SET);

	printf("Writing Data from 0x%.8X...\n", offset);
	while ((tmp = fread(rBuffer, sizeof(unsigned char), sizeof(rBuffer), src)) > 0) {
		if (fwrite(rBuffer, sizeof(unsigned char), tmp, dst) != tmp) {
			printf("Write Error: dst (%.8X)\n", ftell(dst));
			return 1;
		}
	}

	return 0;
}

/*=======================================================*/
/*  dstの現在位置から指定したアドレスまで、0x00データを追加  */
/*  戻り値 エラーは1                                     */
/*=======================================================*/
int addZero(FILE* dst, long offset) {

	unsigned char rBuffer[1024];
	int tmp = 0;
	long padding_size = 0;

	if (dst == NULL) return 1;

	tmp = 0x00;
	memset(rBuffer, tmp, sizeof(rBuffer));
	padding_size = offset - ftell(dst) +1;

	//fseek(dst, 0x0L, SEEK_END);

	printf("Writing Padding up to 0x%.8X...\n", offset);
	while (padding_size > 0) {
		padding_size -= sizeof(rBuffer);

		if (padding_size > 0) fwrite(rBuffer, sizeof(unsigned char), sizeof(rBuffer), dst);
		else fwrite(rBuffer, sizeof(unsigned char), sizeof(rBuffer) + padding_size, dst);
	}
	return 0;

}

/*=================================================*/
/*  dstの指定したoffset1まで、srcのデータを追加し  */
/*  offset2まで0で埋める                           */
/*  戻り値 エラーは1                               */
/*=================================================*/
int addDataAndZero(FILE* dst, FILE* src, long offset1, long offset2) {

	if (dst == NULL) return 1;
	if (src == NULL) return 1;

	if (addData(dst, src, offset1) != 0) return 1;
	if (addZero(dst, offset2) != 0) return 1;
	
	return 0;
}

/*===================================*/
/*  GCMイメージの生成                */
/*  戻り値 エラーは1                 */
/*===================================*/
int genGcmImage(char* file1, char* file2, char* gcmimage){

	FILE *fp, *fp_gcm;
	unsigned char rBuffer[1024];
	int offset = 0, tmp = 0;

	////////////////////////////////////
	// ファイル2を開く(開けるかだけチェック)
	if( (fp = fopen(file2, "rb")) == NULL ){
		printf("Cannot open a %s\n", file2);
		return 1;
	}
	if(fp != NULL) fclose(fp);
	////////////////////////////////////
	// ファイル1を開く
	if( (fp = fopen(file1, "rb")) == NULL ){
		printf("Cannot open a %s\n", file1);
		return 1;
	}

	////////////////////////////////////
	// 保存先のファイルを開く
	if( (fp_gcm = fopen(gcmimage, "wb")) == NULL ){
		printf("Cannot open a %s\n", gcmimage);
		return 1;
	}

	////////////////////////////////////
	// ファイル1のデータをGCMイメージへ書き込み
	if (addDataAndZero(fp_gcm, fp, 0x0, 0x4FFFFFFF) != 0) {
		printf("Error");
		if (fp != NULL) fclose(fp);
		if (fp_gcm != NULL) fclose(fp_gcm);
		return 0;
	}
	if (fp != NULL) fclose(fp);
    
    /*
	////////////////////////////////////
	// POERSAVESの追加データ1をGCMイメージへ書き込み
#define DATA_1 "game1-089-08f.gcm"
#define DATA_2 "game2-091-095.gcm"

	if ((fp = fopen(DATA_1, "rb")) == NULL) {
		printf("Cannot open a %s\n", DATA_1);
		return 1;
	}
	if (addDataAndZero(fp_gcm, fp, 0x08900000, 0x090FFFFF) != 0) {
		printf("Error");
		if (fp != NULL) fclose(fp);
		if (fp_gcm != NULL) fclose(fp_gcm);
		return 0;
	}
	if (fp != NULL) fclose(fp);

	////////////////////////////////////
	// POERSAVESの追加データ2をGCMイメージへ書き込み
	if ((fp = fopen(DATA_2, "rb")) == NULL) {
		printf("Cannot open a %s\n", DATA_2);
		return 1;
	}
	if (addDataAndZero(fp_gcm, fp, 0x09100000, 0x4FFFFFFF) != 0) {
		printf("Error");
		if (fp != NULL) fclose(fp);
		if (fp_gcm != NULL) fclose(fp_gcm);
		return 0;
	}
	if (fp != NULL) fclose(fp);
	*/

	////////////////////////////////////
	// ファイル2のデータをGCMイメージへ書き込み
	if ((fp = fopen(file2, "rb")) == NULL) {
		printf("Cannot open a %s\n", file2);
		return 1;
	}
	if (addDataAndZero(fp_gcm, fp, 0x50000000, 0x57057FFF) != 0) {
		printf("Error");
		if (fp != NULL) fclose(fp);
		if (fp_gcm != NULL) fclose(fp_gcm);
		return 0;
	}
	if (fp != NULL) fclose(fp);

	printf("Generate a AR GCM image: %s\nImage size: %d bytes\n", gcmimage, ftell(fp_gcm));
	if (fp_gcm != NULL) fclose(fp_gcm);

	return 0;
}

/*===================================*/
/*  メイン関数                       */
/*===================================*/
int main(int argc, char* argv[]){
	int mode = 0;
	
	////////////////////////////////////
	// アプリケーション名の表示
	printf("=======================================\n");
	printf(" Action Replay Backup Helper Ver.0.02  \n");
	printf("                       by Nekokabu     \n");
	printf("=======================================\n");

	////////////////////////////////////
	// 引数のチェック
	if( (argc < 3) || (argc > 5) ){
		usage();		
		return 0;
	}

	////////////////////////////////////
	// オプションのチェック
	if(strcmp(argv[1], "-p") == 0) mode = 1;
	if(strcmp(argv[1], "-s") == 0) mode = 2;
	if(strcmp(argv[1], "-gcm") == 0) mode = 3;
	

	if(mode == 1){
		////////////////////////////////////
		// SDカードにパッチ
		printf("Mode: Patch a SD card...\n");
		if( chkSdCard(argv[2]) > 0 ){
			patchSdCard(argv[2]);
		}
	}
	else if(mode == 2){
		if(argc == 4){
			////////////////////////////////////
			// SDカードからデータの読み込み
			printf("Mode: Read AR data from SD card...\n");
			if( chkSdCard(argv[2]) > 0 ){
				ripArData(argv[2], argv[3]);
			}
		}
		else{
			////////////////////////////////////
			// オプションがおかしい
			usage();
			return 0;
		}
	}
	else if(mode == 3){
		if(argc == 5){
			////////////////////////////////////
			// GCMイメージの作成
			printf("Mode: Generate a AR gcm image from data files...\n");
			genGcmImage(argv[2], argv[3], argv[4]);
		}
		else{
			////////////////////////////////////
			// オプションがおかしい
			usage();
			return 0;
		}
	}
	else{
		////////////////////////////////////
		// オプション未設定
		usage();
		return 0;
	}
	printf("Done.\n");
	return 0;
}

