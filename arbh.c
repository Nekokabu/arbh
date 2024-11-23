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
/*  �v���O�����̎g����               */
/*===================================*/
void usage(void){
	printf("usage: arbh {option} {driveletter/filename}...\n\n");
	printf("ex:\n");
	printf(" Patch a SD card:\n");
	printf("    arbh -p F:\n");			//�w�肵��SD�J�[�h�Ƀp�b�`
	printf(" Read AR data from SD card:\n");
	printf("    arbh -s F: OUTPUTFILE.bin\n");	//�w�肵��SD�J�[�h����f�[�^�̕ۑ�
	printf(" Generate a AR gcm image from data files:\n");
	printf("    arbh -gcm INPUTFILE1.bin INPUTFILE2.bin OUTPUTFILE.gcm\n");	//�f�[�^����AR��GCM�t�@�C���𐶐�
	
	getchar();
}

/*========================================================*/
/*  SD�J�[�h��DVDDUMP�Ńt�H�[�}�b�g����Ă��邩�`�F�b�N   */
/*  �߂�l �G���[��0                                      */
/*========================================================*/
int chkSdCard(char* driveletter){
	FILE *fp;
	char rBuffer[20] = "";
	unsigned int size = 0;
	
	printf("Checking SDcard... ");
	if( (strlen(driveletter) == 2) && (driveletter[1] == ':') ){

		////////////////////////////////////
		// wiidump.bin���J��
		sprintf(rBuffer, FILE_WIIDUMP_BIN, driveletter);
		if( (fp = fopen(rBuffer, "rb")) == NULL ){
			printf("Cannot open a %s\n", rBuffer);
			return 0;
		}

		////////////////////////////////////
		// wiidump.bin�̐擪16�o�C�g��ǂݍ���
		fseek(fp, 0x0L, SEEK_SET);
		fread(rBuffer, sizeof(char), 16, fp);

		////////////////////////////////////
		// wiidump.bin�̃t�@�C���T�C�Y�̎擾
		fseek(fp, 0x0L, SEEK_END);
		size = ftell(fp);
		fclose(fp);
		
		////////////////////////////////////
		// "HELLODVDDUMP =]"���ǂ����`�F�b�N
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
/*  SD�J�[�h��Chunksize�Ƀp�b�`      */
/*  �߂�l �G���[��0                 */
/*===================================*/
int patchSdCard(char* driveletter){
	FILE *fp;
	char rBuffer[20] = "";

	////////////////////////////////////
	// wiidump.bin���J��
	sprintf(rBuffer, FILE_WIIDUMP_BIN, driveletter);
	if( (fp = fopen(rBuffer, "rb+")) == NULL ){
		printf("Cannot open a %s\n", rBuffer);
		return 0;
	}
	
	////////////////////////////////////
	// 0x00000040(Chunksize)��0x50000000�ɕύX
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
/*  AR�̃f�[�^���t�@�C���ɕۑ�       */
/*  �߂�l �G���[��0                 */
/*===================================*/
int ripArData(char* driveletter, char* file){
	FILE *fp, *fpt;
	char gameName[0x30] = "";
	unsigned char rBuffer[1024];

	////////////////////////////////////
	// wiidump.bin���J��
	sprintf(gameName, FILE_WIIDUMP_BIN, driveletter);
	if( (fp = fopen(gameName, "rb")) == NULL ){
		printf("Cannot open a %s\n", gameName);
		return 0;
	}

	////////////////////////////////////
	// �ۑ���̃t�@�C�����J��
	if( (fpt = fopen(file, "wb")) == NULL ){
		printf("Cannot open a %s\n", file);
		return 0;
	}
	
	////////////////////////////////////
	// �Q�[�����̎擾
	fseek(fp, 0x10L, SEEK_SET);
	fread(gameName, sizeof(char), 0x30, fp);
	if( strcmp(gameName, "NoGame") == 0){
		printf("There are not dumped data in SDcard!\n");
		return 0;
	}
	printf("Disc Title: %s\n", gameName);
	
	//////////////////////////////////////////////////
	////////////////////////////////////
	// wiidump.bin�̃`�F�b�N�B
	//  �p�b�`���������Ă邩
	fseek(fp, 0x40L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x50 ) || ( rBuffer[1] != 0x00 ) || ( rBuffer[2] != 0x00 ) || ( rBuffer[3] != 0x00 ) ){
		printf("SDcard does not apply a patch.\n Please use \"-p\" option.\n");
		return 0;
	}

	////////////////////////////////////
	// wiidump.bin�̃`�F�b�N�B
	//  �f�B�X�N�T�C�Y(LBA)�̃`�F�b�N�B
	fseek(fp, 0x48L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x00 ) || ( rBuffer[1] != 0x0A ) || ( rBuffer[2] != 0xE0 ) || ( rBuffer[3] != 0xB0 ) ){
		printf("SDcard data is not dumped from GC disc.\n");
		return 0;
	}

	////////////////////////////////////
	// wiidump.bin�̃`�F�b�N�B
	//  �z�o���ɐ����������t���O�̃`�F�b�N
	//  ��DVDDUMP�ŃG���[���o���ɐ���Ƀ_���v���ł��Ă����0x00000053��1�ɂȂ�B
	//    AR�z���o���ꍇ�͂������K��0�ɂȂ�
	fseek(fp, 0x50L, SEEK_SET);
	fread(rBuffer, sizeof(unsigned char), 4, fp);
	if( (rBuffer[0] != 0x00 ) || ( rBuffer[1] != 0x00 ) || ( rBuffer[2] != 0x00 ) || ( rBuffer[3] != 0x00 ) ){
		printf("SDcard data is not dumped from AR disc.\n");
		return 0;
	}

	//////////////////////////////////////////////////
	////////////////////////////////////
	// �_���v���ꂽ�f�[�^���t�@�C���ɕۑ�
	fseek(fp, 0x200L, SEEK_SET);
	fseek(fpt, 0x0L, SEEK_SET);
	
	while(1){
		if( fread(rBuffer, sizeof(unsigned char), sizeof(rBuffer), fp) < sizeof(rBuffer) ){
			printf("Read Error: %.8X\n", ftell(fp));
			break;
		}
		
		////////////////////////////////////
		// �G���[�Z�N�^(���ׂẴf�[�^��0xA8)�̌��o
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
/*  dst�̎w�肵���A�h���X�ɁAsrc�̃f�[�^��ǉ�  */
/*  �߂�l �G���[��1                            */
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
/*  dst�̌��݈ʒu����w�肵���A�h���X�܂ŁA0x00�f�[�^��ǉ�  */
/*  �߂�l �G���[��1                                     */
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
/*  dst�̎w�肵��offset1�܂ŁAsrc�̃f�[�^��ǉ���  */
/*  offset2�܂�0�Ŗ��߂�                           */
/*  �߂�l �G���[��1                               */
/*=================================================*/
int addDataAndZero(FILE* dst, FILE* src, long offset1, long offset2) {

	if (dst == NULL) return 1;
	if (src == NULL) return 1;

	if (addData(dst, src, offset1) != 0) return 1;
	if (addZero(dst, offset2) != 0) return 1;
	
	return 0;
}

/*===================================*/
/*  GCM�C���[�W�̐���                */
/*  �߂�l �G���[��1                 */
/*===================================*/
int genGcmImage(char* file1, char* file2, char* gcmimage){

	FILE *fp, *fp_gcm;
	unsigned char rBuffer[1024];
	int offset = 0, tmp = 0;

	////////////////////////////////////
	// �t�@�C��2���J��(�J���邩�����`�F�b�N)
	if( (fp = fopen(file2, "rb")) == NULL ){
		printf("Cannot open a %s\n", file2);
		return 1;
	}
	if(fp != NULL) fclose(fp);
	////////////////////////////////////
	// �t�@�C��1���J��
	if( (fp = fopen(file1, "rb")) == NULL ){
		printf("Cannot open a %s\n", file1);
		return 1;
	}

	////////////////////////////////////
	// �ۑ���̃t�@�C�����J��
	if( (fp_gcm = fopen(gcmimage, "wb")) == NULL ){
		printf("Cannot open a %s\n", gcmimage);
		return 1;
	}

	////////////////////////////////////
	// �t�@�C��1�̃f�[�^��GCM�C���[�W�֏�������
	if (addDataAndZero(fp_gcm, fp, 0x0, 0x4FFFFFFF) != 0) {
		printf("Error");
		if (fp != NULL) fclose(fp);
		if (fp_gcm != NULL) fclose(fp_gcm);
		return 0;
	}
	if (fp != NULL) fclose(fp);
    
    /*
	////////////////////////////////////
	// POERSAVES�̒ǉ��f�[�^1��GCM�C���[�W�֏�������
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
	// POERSAVES�̒ǉ��f�[�^2��GCM�C���[�W�֏�������
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
	// �t�@�C��2�̃f�[�^��GCM�C���[�W�֏�������
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
/*  ���C���֐�                       */
/*===================================*/
int main(int argc, char* argv[]){
	int mode = 0;
	
	////////////////////////////////////
	// �A�v���P�[�V�������̕\��
	printf("=======================================\n");
	printf(" Action Replay Backup Helper Ver.0.02  \n");
	printf("                       by Nekokabu     \n");
	printf("=======================================\n");

	////////////////////////////////////
	// �����̃`�F�b�N
	if( (argc < 3) || (argc > 5) ){
		usage();		
		return 0;
	}

	////////////////////////////////////
	// �I�v�V�����̃`�F�b�N
	if(strcmp(argv[1], "-p") == 0) mode = 1;
	if(strcmp(argv[1], "-s") == 0) mode = 2;
	if(strcmp(argv[1], "-gcm") == 0) mode = 3;
	

	if(mode == 1){
		////////////////////////////////////
		// SD�J�[�h�Ƀp�b�`
		printf("Mode: Patch a SD card...\n");
		if( chkSdCard(argv[2]) > 0 ){
			patchSdCard(argv[2]);
		}
	}
	else if(mode == 2){
		if(argc == 4){
			////////////////////////////////////
			// SD�J�[�h����f�[�^�̓ǂݍ���
			printf("Mode: Read AR data from SD card...\n");
			if( chkSdCard(argv[2]) > 0 ){
				ripArData(argv[2], argv[3]);
			}
		}
		else{
			////////////////////////////////////
			// �I�v�V��������������
			usage();
			return 0;
		}
	}
	else if(mode == 3){
		if(argc == 5){
			////////////////////////////////////
			// GCM�C���[�W�̍쐬
			printf("Mode: Generate a AR gcm image from data files...\n");
			genGcmImage(argv[2], argv[3], argv[4]);
		}
		else{
			////////////////////////////////////
			// �I�v�V��������������
			usage();
			return 0;
		}
	}
	else{
		////////////////////////////////////
		// �I�v�V�������ݒ�
		usage();
		return 0;
	}
	printf("Done.\n");
	return 0;
}

