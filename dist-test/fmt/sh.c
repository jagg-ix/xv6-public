7600 // Shell.
7601 
7602 #include "types.h"
7603 #include "user.h"
7604 #include "fcntl.h"
7605 
7606 // Parsed command representation
7607 #define EXEC  1
7608 #define REDIR 2
7609 #define PIPE  3
7610 #define LIST  4
7611 #define BACK  5
7612 
7613 #define MAXARGS 10
7614 
7615 struct cmd {
7616   int type;
7617 };
7618 
7619 struct execcmd {
7620   int type;
7621   char *argv[MAXARGS];
7622   char *eargv[MAXARGS];
7623 };
7624 
7625 struct redircmd {
7626   int type;
7627   struct cmd *cmd;
7628   char *file;
7629   char *efile;
7630   int mode;
7631   int fd;
7632 };
7633 
7634 struct pipecmd {
7635   int type;
7636   struct cmd *left;
7637   struct cmd *right;
7638 };
7639 
7640 struct listcmd {
7641   int type;
7642   struct cmd *left;
7643   struct cmd *right;
7644 };
7645 
7646 struct backcmd {
7647   int type;
7648   struct cmd *cmd;
7649 };
7650 int fork1(void);  // Fork but panics on failure.
7651 void panic(char*);
7652 struct cmd *parsecmd(char*);
7653 
7654 // Execute cmd.  Never returns.
7655 void
7656 runcmd(struct cmd *cmd)
7657 {
7658   int p[2];
7659   struct backcmd *bcmd;
7660   struct execcmd *ecmd;
7661   struct listcmd *lcmd;
7662   struct pipecmd *pcmd;
7663   struct redircmd *rcmd;
7664 
7665   if(cmd == 0)
7666     exit();
7667 
7668   switch(cmd->type){
7669   default:
7670     panic("runcmd");
7671 
7672   case EXEC:
7673     ecmd = (struct execcmd*)cmd;
7674     if(ecmd->argv[0] == 0)
7675       exit();
7676     exec(ecmd->argv[0], ecmd->argv);
7677     printf(2, "exec %s failed\n", ecmd->argv[0]);
7678     break;
7679 
7680   case REDIR:
7681     rcmd = (struct redircmd*)cmd;
7682     close(rcmd->fd);
7683     if(open(rcmd->file, rcmd->mode) < 0){
7684       printf(2, "open %s failed\n", rcmd->file);
7685       exit();
7686     }
7687     runcmd(rcmd->cmd);
7688     break;
7689 
7690   case LIST:
7691     lcmd = (struct listcmd*)cmd;
7692     if(fork1() == 0)
7693       runcmd(lcmd->left);
7694     wait();
7695     runcmd(lcmd->right);
7696     break;
7697 
7698 
7699 
7700   case PIPE:
7701     pcmd = (struct pipecmd*)cmd;
7702     if(pipe(p) < 0)
7703       panic("pipe");
7704     if(fork1() == 0){
7705       close(1);
7706       dup(p[1]);
7707       close(p[0]);
7708       close(p[1]);
7709       runcmd(pcmd->left);
7710     }
7711     if(fork1() == 0){
7712       close(0);
7713       dup(p[0]);
7714       close(p[0]);
7715       close(p[1]);
7716       runcmd(pcmd->right);
7717     }
7718     close(p[0]);
7719     close(p[1]);
7720     wait();
7721     wait();
7722     break;
7723 
7724   case BACK:
7725     bcmd = (struct backcmd*)cmd;
7726     if(fork1() == 0)
7727       runcmd(bcmd->cmd);
7728     break;
7729   }
7730   exit();
7731 }
7732 
7733 int
7734 getcmd(char *buf, int nbuf)
7735 {
7736   printf(2, "$ ");
7737   memset(buf, 0, nbuf);
7738   gets(buf, nbuf);
7739   if(buf[0] == 0) // EOF
7740     return -1;
7741   return 0;
7742 }
7743 
7744 
7745 
7746 
7747 
7748 
7749 
7750 int
7751 main(void)
7752 {
7753   static char buf[100];
7754   int fd;
7755 
7756   // Assumes three file descriptors open.
7757   while((fd = open("console", O_RDWR)) >= 0){
7758     if(fd >= 3){
7759       close(fd);
7760       break;
7761     }
7762   }
7763 
7764   // Read and run input commands.
7765   while(getcmd(buf, sizeof(buf)) >= 0){
7766     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
7767       // Clumsy but will have to do for now.
7768       // Chdir has no effect on the parent if run in the child.
7769       buf[strlen(buf)-1] = 0;  // chop \n
7770       if(chdir(buf+3) < 0)
7771         printf(2, "cannot cd %s\n", buf+3);
7772       continue;
7773     }
7774     if(fork1() == 0)
7775       runcmd(parsecmd(buf));
7776     wait();
7777   }
7778   exit();
7779 }
7780 
7781 void
7782 panic(char *s)
7783 {
7784   printf(2, "%s\n", s);
7785   exit();
7786 }
7787 
7788 int
7789 fork1(void)
7790 {
7791   int pid;
7792 
7793   pid = fork();
7794   if(pid == -1)
7795     panic("fork");
7796   return pid;
7797 }
7798 
7799 
7800 // Constructors
7801 
7802 struct cmd*
7803 execcmd(void)
7804 {
7805   struct execcmd *cmd;
7806 
7807   cmd = malloc(sizeof(*cmd));
7808   memset(cmd, 0, sizeof(*cmd));
7809   cmd->type = EXEC;
7810   return (struct cmd*)cmd;
7811 }
7812 
7813 struct cmd*
7814 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
7815 {
7816   struct redircmd *cmd;
7817 
7818   cmd = malloc(sizeof(*cmd));
7819   memset(cmd, 0, sizeof(*cmd));
7820   cmd->type = REDIR;
7821   cmd->cmd = subcmd;
7822   cmd->file = file;
7823   cmd->efile = efile;
7824   cmd->mode = mode;
7825   cmd->fd = fd;
7826   return (struct cmd*)cmd;
7827 }
7828 
7829 struct cmd*
7830 pipecmd(struct cmd *left, struct cmd *right)
7831 {
7832   struct pipecmd *cmd;
7833 
7834   cmd = malloc(sizeof(*cmd));
7835   memset(cmd, 0, sizeof(*cmd));
7836   cmd->type = PIPE;
7837   cmd->left = left;
7838   cmd->right = right;
7839   return (struct cmd*)cmd;
7840 }
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
7850 struct cmd*
7851 listcmd(struct cmd *left, struct cmd *right)
7852 {
7853   struct listcmd *cmd;
7854 
7855   cmd = malloc(sizeof(*cmd));
7856   memset(cmd, 0, sizeof(*cmd));
7857   cmd->type = LIST;
7858   cmd->left = left;
7859   cmd->right = right;
7860   return (struct cmd*)cmd;
7861 }
7862 
7863 struct cmd*
7864 backcmd(struct cmd *subcmd)
7865 {
7866   struct backcmd *cmd;
7867 
7868   cmd = malloc(sizeof(*cmd));
7869   memset(cmd, 0, sizeof(*cmd));
7870   cmd->type = BACK;
7871   cmd->cmd = subcmd;
7872   return (struct cmd*)cmd;
7873 }
7874 // Parsing
7875 
7876 char whitespace[] = " \t\r\n\v";
7877 char symbols[] = "<|>&;()";
7878 
7879 int
7880 gettoken(char **ps, char *es, char **q, char **eq)
7881 {
7882   char *s;
7883   int ret;
7884 
7885   s = *ps;
7886   while(s < es && strchr(whitespace, *s))
7887     s++;
7888   if(q)
7889     *q = s;
7890   ret = *s;
7891   switch(*s){
7892   case 0:
7893     break;
7894   case '|':
7895   case '(':
7896   case ')':
7897   case ';':
7898   case '&':
7899   case '<':
7900     s++;
7901     break;
7902   case '>':
7903     s++;
7904     if(*s == '>'){
7905       ret = '+';
7906       s++;
7907     }
7908     break;
7909   default:
7910     ret = 'a';
7911     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
7912       s++;
7913     break;
7914   }
7915   if(eq)
7916     *eq = s;
7917 
7918   while(s < es && strchr(whitespace, *s))
7919     s++;
7920   *ps = s;
7921   return ret;
7922 }
7923 
7924 int
7925 peek(char **ps, char *es, char *toks)
7926 {
7927   char *s;
7928 
7929   s = *ps;
7930   while(s < es && strchr(whitespace, *s))
7931     s++;
7932   *ps = s;
7933   return *s && strchr(toks, *s);
7934 }
7935 
7936 
7937 
7938 
7939 
7940 
7941 
7942 
7943 
7944 
7945 
7946 
7947 
7948 
7949 
7950 struct cmd *parseline(char**, char*);
7951 struct cmd *parsepipe(char**, char*);
7952 struct cmd *parseexec(char**, char*);
7953 struct cmd *nulterminate(struct cmd*);
7954 
7955 struct cmd*
7956 parsecmd(char *s)
7957 {
7958   char *es;
7959   struct cmd *cmd;
7960 
7961   es = s + strlen(s);
7962   cmd = parseline(&s, es);
7963   peek(&s, es, "");
7964   if(s != es){
7965     printf(2, "leftovers: %s\n", s);
7966     panic("syntax");
7967   }
7968   nulterminate(cmd);
7969   return cmd;
7970 }
7971 
7972 struct cmd*
7973 parseline(char **ps, char *es)
7974 {
7975   struct cmd *cmd;
7976 
7977   cmd = parsepipe(ps, es);
7978   while(peek(ps, es, "&")){
7979     gettoken(ps, es, 0, 0);
7980     cmd = backcmd(cmd);
7981   }
7982   if(peek(ps, es, ";")){
7983     gettoken(ps, es, 0, 0);
7984     cmd = listcmd(cmd, parseline(ps, es));
7985   }
7986   return cmd;
7987 }
7988 
7989 
7990 
7991 
7992 
7993 
7994 
7995 
7996 
7997 
7998 
7999 
8000 struct cmd*
8001 parsepipe(char **ps, char *es)
8002 {
8003   struct cmd *cmd;
8004 
8005   cmd = parseexec(ps, es);
8006   if(peek(ps, es, "|")){
8007     gettoken(ps, es, 0, 0);
8008     cmd = pipecmd(cmd, parsepipe(ps, es));
8009   }
8010   return cmd;
8011 }
8012 
8013 struct cmd*
8014 parseredirs(struct cmd *cmd, char **ps, char *es)
8015 {
8016   int tok;
8017   char *q, *eq;
8018 
8019   while(peek(ps, es, "<>")){
8020     tok = gettoken(ps, es, 0, 0);
8021     if(gettoken(ps, es, &q, &eq) != 'a')
8022       panic("missing file for redirection");
8023     switch(tok){
8024     case '<':
8025       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
8026       break;
8027     case '>':
8028       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8029       break;
8030     case '+':  // >>
8031       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8032       break;
8033     }
8034   }
8035   return cmd;
8036 }
8037 
8038 
8039 
8040 
8041 
8042 
8043 
8044 
8045 
8046 
8047 
8048 
8049 
8050 struct cmd*
8051 parseblock(char **ps, char *es)
8052 {
8053   struct cmd *cmd;
8054 
8055   if(!peek(ps, es, "("))
8056     panic("parseblock");
8057   gettoken(ps, es, 0, 0);
8058   cmd = parseline(ps, es);
8059   if(!peek(ps, es, ")"))
8060     panic("syntax - missing )");
8061   gettoken(ps, es, 0, 0);
8062   cmd = parseredirs(cmd, ps, es);
8063   return cmd;
8064 }
8065 
8066 struct cmd*
8067 parseexec(char **ps, char *es)
8068 {
8069   char *q, *eq;
8070   int tok, argc;
8071   struct execcmd *cmd;
8072   struct cmd *ret;
8073 
8074   if(peek(ps, es, "("))
8075     return parseblock(ps, es);
8076 
8077   ret = execcmd();
8078   cmd = (struct execcmd*)ret;
8079 
8080   argc = 0;
8081   ret = parseredirs(ret, ps, es);
8082   while(!peek(ps, es, "|)&;")){
8083     if((tok=gettoken(ps, es, &q, &eq)) == 0)
8084       break;
8085     if(tok != 'a')
8086       panic("syntax");
8087     cmd->argv[argc] = q;
8088     cmd->eargv[argc] = eq;
8089     argc++;
8090     if(argc >= MAXARGS)
8091       panic("too many args");
8092     ret = parseredirs(ret, ps, es);
8093   }
8094   cmd->argv[argc] = 0;
8095   cmd->eargv[argc] = 0;
8096   return ret;
8097 }
8098 
8099 
8100 // NUL-terminate all the counted strings.
8101 struct cmd*
8102 nulterminate(struct cmd *cmd)
8103 {
8104   int i;
8105   struct backcmd *bcmd;
8106   struct execcmd *ecmd;
8107   struct listcmd *lcmd;
8108   struct pipecmd *pcmd;
8109   struct redircmd *rcmd;
8110 
8111   if(cmd == 0)
8112     return 0;
8113 
8114   switch(cmd->type){
8115   case EXEC:
8116     ecmd = (struct execcmd*)cmd;
8117     for(i=0; ecmd->argv[i]; i++)
8118       *ecmd->eargv[i] = 0;
8119     break;
8120 
8121   case REDIR:
8122     rcmd = (struct redircmd*)cmd;
8123     nulterminate(rcmd->cmd);
8124     *rcmd->efile = 0;
8125     break;
8126 
8127   case PIPE:
8128     pcmd = (struct pipecmd*)cmd;
8129     nulterminate(pcmd->left);
8130     nulterminate(pcmd->right);
8131     break;
8132 
8133   case LIST:
8134     lcmd = (struct listcmd*)cmd;
8135     nulterminate(lcmd->left);
8136     nulterminate(lcmd->right);
8137     break;
8138 
8139   case BACK:
8140     bcmd = (struct backcmd*)cmd;
8141     nulterminate(bcmd->cmd);
8142     break;
8143   }
8144   return cmd;
8145 }
8146 
8147 
8148 
8149 
