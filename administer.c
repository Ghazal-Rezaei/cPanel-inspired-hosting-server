#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

void EditUsr(void);
void SuspendUsr(void);
int ListManageUsers(void);
int MakeNewAdmin(void);
int SSL(char *domain, char *email);
int VirtualHost(char *username);
int AdjustSize(char *username);
int MakeNewUser(void);


int main()
{
	char Continue[5] = {};
	do{
		system("clear");
		int CommandNo = 0, state = 0, i = 0;
		printf("What do you want to do?\n");
		printf("1-Make a new user\n2-Make a new admin\n3-List and manage users\n");
		printf("please enter the number of your desired command:\n");
		scanf("%d", &CommandNo);
		getchar();

		switch(CommandNo){
			case 1:
				state = MakeNewUser();
				if(!state)
					printf("Something went wrong!\n");
				break;
			case 2:
				state = MakeNewAdmin();
				if(!state)
					printf("Something went wrong!\n");
				break;
			case 3:
				state = ListManageUsers();
				if(!state)
					printf("Something went wrong!\n");
				break;
		}

		printf("Do you want to continue? [yes/no]\n");
		fgets(Continue, sizeof(Continue), stdin);
		Continue[strlen(Continue)-1] = 0;
		for(i = 0; i < strlen(Continue); i++)
			Continue[i] = tolower(Continue[i]);
	
	}while(strcmp(Continue,"no") && strcmp(Continue,"n"));
	srand(time(NULL));
return 0;
}


int VirtualHost(char *username)
{
	char domain[150] = {}, command[300] = {}, email[200] = {};
	FILE* handler;
	int s = 0;
	printf("please enter the domain of the new user:\n");
	fgets(domain, sizeof(domain), stdin);
	domain[strlen(domain)-1] = 0;
	sprintf(command, "/var/www/html/%s/public_html/index.html", domain);
	if(access(command,F_OK) != -1){
		printf("domain already exists!\n");
		return 0;
	}

	sprintf(command, "sudo mkdir -p /var/www/html/%s/public_html", domain);
	system(command);
	sprintf(command, "sudo chown -R %s:%s_g /var/www/html/%s/public_html", username, username, domain);
	system(command);
	sprintf(command, "sudo chmod -R 775 /var/www/html/");
	system(command);	
	sprintf(command, "sudo chmod -R 770 /var/www/html/%s/public_html", domain);
	system(command);
	sprintf(command, "sudo setfacl -Rdm g:%s_g:rwx /var/www/html/%s/public_html", username, domain);
	system(command);
	sprintf(command, "sudo chmod g+s /var/www/html/%s/public_html", domain);
	system(command);

	sprintf(command, "/var/www/html/%s/public_html/index.html", domain);
	handler = fopen(command,"w");
	if (handler == NULL){
		return 0;
	}
	fprintf(handler,"<html>\n<body>\n<h1>Hi there, it's %s</h1>\n</body>\n</html>", domain);
	fclose(handler);

	printf("please enter the email of the new user:\n");
	fgets(email, sizeof(email), stdin);
	email[strlen(email)-1] = 0;

	printf("Do you need SSL for this domain? [yes/no]\n");
	fgets(command, sizeof(command), stdin);
	command[strlen(command)-1] = 0;
	if(!strcmp(command,"yes") || !strcmp(command,"y")){
		s = 1;
		if(!SSL(domain,email)) return 0;
	}
	
	sprintf(command, "/etc/apache2/sites-available/%s.conf", domain);
	handler = fopen(command,"w");
	if (handler == NULL){
		return 0;
	}
	fprintf(handler,"<VirtualHost *:80>\n");
	fprintf(handler,"ServerAdmin %s\n", email);
	fprintf(handler,"ServerName %s\n", domain);
	fprintf(handler,"ServerAlias www.%s\n", domain);
	fprintf(handler,"DocumentRoot /var/www/html/%s/public_html\n", domain);
	if(s) fprintf(handler,"Redirect permanent \"/\" \"https://%s/\"\n", domain);
	fprintf(handler,"ErrorLog ${APACHE_LOG_DIR}/error.log\nCustomLog ${APACHE_LOG_DIR}/access.log combined\n");
	fprintf(handler,"</VirtualHost>");
	fclose(handler);

	sprintf(command, "sudo a2ensite %s.conf", domain);
	system(command);
	sprintf(command, "sudo systemctl reload apache2");
	system(command);

	handler = fopen("/etc/hosts","a");
	if (handler == NULL)
		return 0;
	fprintf(handler,"127.0.2.1\t%s\n", domain);
	return 1;
}


int SSL(char *domain, char *email)
{
	char command[255] = {};	
	FILE *handler;
	sprintf(command, "/etc/apache2/sites-available/default-ssl.conf");
	handler = fopen(command,"a");
	if (handler == NULL){
		return 0;
	}
	fprintf(handler,"<VirtualHost *:443>\n");
	fprintf(handler,"ServerAdmin %s\n", email);
	fprintf(handler,"ServerName %s\n", domain);
	fprintf(handler,"ServerAlias www.%s\n", domain);
	fprintf(handler,"DocumentRoot /var/www/html/%s/public_html\n", domain);
	fprintf(handler,"ErrorLog ${APACHE_LOG_DIR}/error.log\nCustomLog ${APACHE_LOG_DIR}/access.log combined\n");
	fprintf(handler,"SSLEngine on\n");
	fprintf(handler,"SSLCertificateFile	/etc/ssl/certs/apache-selfsigned.crt\n");
	fprintf(handler,"SSLCertificateKeyFile /etc/ssl/private/apache-selfsigned.key\n");
	fprintf(handler,"<FilesMatch \"\\.(cgi|shtml|phtml|php)$\">\nSSLOptions +StdEnvVars\n</FilesMatch>\n");
	fprintf(handler,"<Directory /usr/lib/cgi-bin>\nSSLOptions +StdEnvVars\n</Directory>\n");
	fprintf(handler,"BrowserMatch \"MSIE [2-6]\" \\\n nokeepalive ssl-unclean-shutdown \\\ndowngrade-1.0 force-response-1.0\n");
	fprintf(handler,"</VirtualHost>\n");
	fclose(handler);
	return 1;
}


int AdjustSize(char *username)
{
	char command[250] = {};
	int SoftLimit = 0,HardLimit = 0;
	printf("please enter the soft and hard memory limit of the new user respectively:\n");
	scanf("%d%d", &SoftLimit, &HardLimit);
	getchar();
	if(SoftLimit > HardLimit || HardLimit > 120)
		return 0;
	sprintf(command,"sudo setquota -u %s %dM %dM 0 0 /", username, SoftLimit, HardLimit);
	system(command);
	return 1;
}


int MakeNewUser(void)
{
	char username[100] = {}, command[250] = {}, userinfo[500] = {};
	char Uname[100] = {}, Passwd[255] = {};
	FILE* handler;

	printf("please enter the username of the new user:\n");
	fgets(username, sizeof(username), stdin);
	username[strlen(username)-1] = 0;
	handler = fopen("/etc/shadow","r");
	if (handler == NULL)
		return 0;
	while(fscanf(handler, "%495s", userinfo) != EOF){
		strncpy(Uname,strtok(userinfo,":"),98);
		if(!strcmp(Uname,username)){
			fclose(handler);
			return 0;		
		}	
	}
	fclose(handler);
	sprintf(command,"sudo useradd -m %s", username);
	system(command);
	sprintf(command,"sudo groupadd %s_g", username);
	system(command);
	sprintf(command,"sudo usermod -a -G %s_g %s", username, username);
	system(command);
	sprintf(command,"sudo usermod -a -G %s_g root", username);
	system(command);
	sprintf(command,"sudo usermod -a -G %s_g www-data", username);
	system(command);
	sprintf(command,"sudo chown -R %s:%s_g /home/%s", username, username, username);
	system(command);
	sprintf(command,"sudo chmod -R 770 /home/%s", username);
	system(command);
	sprintf(command, "sudo setfacl -Rdm g:%s_g:rwx /home/%s", username, username);
	system(command);
	sprintf(command, "sudo chmod g+s /home/%s", username);
	system(command);

	sprintf(command,"sudo passwd %s", username);
	system(command);
	handler = fopen("/etc/shadow","r");
	if (handler == NULL){
		sprintf(command,"sudo userdel -r %s", username);
		system(command);
		return 0;
	}
	while(fscanf(handler, "%495s", userinfo) != EOF){
		strncpy(Uname,strtok(userinfo,":"),98);
		if(!strcmp(Uname,username)){
			strncpy(Passwd,strtok(NULL,":"),198);
			if(!strcmp(Passwd,"!")){
			fclose(handler);
			sprintf(command,"sudo userdel -r %s", username);
			system(command);
			return 0;
			}		
		}	
	}
	fclose(handler);

	sprintf(command,"sudo usermod -g users0 %s", username);
	system(command);
	int s = 0;
	s = VirtualHost(username);
	if(!s) return 0;
	return AdjustSize(username);
}


int MakeNewAdmin(void)
{
	char username[100] = {}, command[150] = {}, userinfo[500] = {};
	char Uname[100] = {}, Passwd[255] = {};
	FILE* handler;

	printf("please enter the username of the new admin:\n");
	fgets(username, sizeof(username), stdin);
	username[strlen(username)-1] = 0;
	handler = fopen("/etc/shadow","r");
	if (handler == NULL)
		return 0;
	while(fscanf(handler, "%495s", userinfo) != EOF){
		strncpy(Uname,strtok(userinfo,":"),98);
		if(!strcmp(Uname,username)){
			fclose(handler);
			return 0;		
		}	
	}
	fclose(handler);
	sprintf(command,"sudo useradd -m %s", username);
	system(command);
	sprintf(command,"sudo chown -R %s /home/%s", username, username);
	system(command);
	sprintf(command,"sudo chmod -R 700 /home/%s", username);
	system(command);

	sprintf(command,"sudo passwd %s", username);
	system(command);
	handler = fopen("/etc/shadow","r");
	if (handler == NULL){
		sprintf(command,"sudo userdel -r %s", username);
		system(command);
		return 0;
	}
	while(fscanf(handler, "%495s", userinfo) != EOF){
		strncpy(Uname,strtok(userinfo,":"),98);
		if(!strcmp(Uname,username)){
			strncpy(Passwd,strtok(NULL,":"),198);
			if(!strcmp(Passwd,"!")){
			fclose(handler);
			sprintf(command,"sudo userdel -r %s", username);
			system(command);
			return 0;
			}		
		}	
	}
	fclose(handler);

	sprintf(command,"sudo usermod -a -G admins %s", username);
	system(command);
	sprintf(command,"sudo adduser %s sudo", username);
	system(command);
	return 1;
}


int ListManageUsers(void)
{
	char s = 0;
	int c = 0;	
	s = system("members users0");
	if(s) return 0;
	printf("What do you want to do?\n");
	printf("1-Suspend a user\n2-Edit some user's info\n3-Return\n");
	scanf("%d", &c);
	getchar();
	if(c == 1) SuspendUsr();
	else if(c == 2) EditUsr();
	return 1;
}


void SuspendUsr(void)
{
	int rtemp = 0, len = 0, i = 0;
	char NewPass[15] = {}, username[100] = {};

	printf("please enter the username of the user you want to suspend:\n");
	fgets(username, sizeof(username), stdin);
	username[strlen(username)-1] = 0;

	len = rand()%8+5;
	for(i = 0; i < len; i++){
		NewPass[i] = 'a'+rand()%26;
		NewPass[i+1] = 0;
	}
	printf("%s", NewPass);
}


void EditUsr(void)
{
}
