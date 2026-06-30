#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char gunforce_decryption_table[];
extern unsigned char bomberman_decryption_table[];
extern unsigned char lethalth_decryption_table[];
extern unsigned char dynablaster_decryption_table[];
extern unsigned char mysticri_decryption_table[];
extern unsigned char majtitl2_decryption_table[];
extern unsigned char hook_decryption_table[];
extern unsigned char rtypeleo_decryption_table[];
extern unsigned char inthunt_decryption_table[];
extern unsigned char gussun_decryption_table[];
extern unsigned char leagueman_decryption_table[];
extern unsigned char psoldier_decryption_table[];
extern unsigned char dsoccr94_decryption_table[];
extern unsigned char shisen2_decryption_table[];

extern void irem_cpu_decrypt(int cpu,unsigned char *decryption_table);
extern void irem_cpu_decrypt_small(int cpu,unsigned char *decryption_table);
#ifdef __cplusplus
}
#endif
