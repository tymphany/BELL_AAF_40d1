/****************************************************************************
Generated from E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\dfu-public.key
by E:\QCC5121\EVK\qcc512x-qcc302x-40d1\adk\tools\packages\menus\dfu_setup.py
at 19:50:12 30/04/2020

FILE NAME
    rsa_pss_constants.c

DESCRIPTION
    Constant data needed for the rsa_decrypt and the ce_pkcs1_pss_padding_verify
    function.

NOTES
    The constant data for the rsa_decrypt is generated by the host from the
    RSA public key and compiled into the rsa_pss_constants library.
    It is directly related to the RSA private and public key pair, and if they
    change then this file must be regenerated from the public key.

****************************************************************************/

#include "rsa_decrypt.h"

/*
 * The const rsa_mod_t *mod parameter for the rsa_decrypt function.
 */
const rsa_mod_t rsa_decrypt_constant_mod = {
    /* uint16 M[RSA_SIGNATURE_SIZE] where RSA_SIGNATURE_SIZE is 128 */
    {
        0xb4f8,     /* 00 */
        0x0ff3,     /* 01 */
        0x5121,     /* 02 */
        0x42e9,     /* 03 */
        0x6bc4,     /* 04 */
        0x94ff,     /* 05 */
        0x814d,     /* 06 */
        0xfd33,     /* 07 */
        0x48b9,     /* 08 */
        0x2bd1,     /* 09 */
        0xc6bd,     /* 10 */
        0xe3de,     /* 11 */
        0x0b30,     /* 12 */
        0x8b3d,     /* 13 */
        0xc420,     /* 14 */
        0x5220,     /* 15 */
        0xae3f,     /* 16 */
        0x822a,     /* 17 */
        0xc1c8,     /* 18 */
        0x99e1,     /* 19 */
        0x7fab,     /* 20 */
        0x23c5,     /* 21 */
        0x868b,     /* 22 */
        0xf7b4,     /* 23 */
        0x1f32,     /* 24 */
        0xa633,     /* 25 */
        0x2b3d,     /* 26 */
        0x2cb9,     /* 27 */
        0xadd4,     /* 28 */
        0xb496,     /* 29 */
        0xc61f,     /* 30 */
        0x5586,     /* 31 */
        0xfedd,     /* 32 */
        0xcc21,     /* 33 */
        0x65f8,     /* 34 */
        0x19b3,     /* 35 */
        0x5c83,     /* 36 */
        0x8892,     /* 37 */
        0xb691,     /* 38 */
        0x30cd,     /* 39 */
        0x9206,     /* 40 */
        0x31e7,     /* 41 */
        0x39f5,     /* 42 */
        0x91f9,     /* 43 */
        0x1102,     /* 44 */
        0x3714,     /* 45 */
        0xdc89,     /* 46 */
        0x5171,     /* 47 */
        0xef22,     /* 48 */
        0xca04,     /* 49 */
        0x6a2b,     /* 50 */
        0x637c,     /* 51 */
        0x706b,     /* 52 */
        0xc78f,     /* 53 */
        0x5956,     /* 54 */
        0xecaa,     /* 55 */
        0xb4a4,     /* 56 */
        0xcd24,     /* 57 */
        0x0fb1,     /* 58 */
        0x6cf9,     /* 59 */
        0xf2f5,     /* 60 */
        0xb5b0,     /* 61 */
        0xad95,     /* 62 */
        0x5b44,     /* 63 */
        0x78b5,     /* 64 */
        0x978f,     /* 65 */
        0xbc20,     /* 66 */
        0xcf4d,     /* 67 */
        0xcf36,     /* 68 */
        0xc43a,     /* 69 */
        0x7917,     /* 70 */
        0x9874,     /* 71 */
        0x9a9c,     /* 72 */
        0xc684,     /* 73 */
        0xa046,     /* 74 */
        0xc7c7,     /* 75 */
        0xb63c,     /* 76 */
        0x1a10,     /* 77 */
        0x2086,     /* 78 */
        0x33a6,     /* 79 */
        0x790d,     /* 80 */
        0xaa60,     /* 81 */
        0xfe38,     /* 82 */
        0x4b96,     /* 83 */
        0x5f90,     /* 84 */
        0x0c77,     /* 85 */
        0x7a90,     /* 86 */
        0xbf78,     /* 87 */
        0x4d1b,     /* 88 */
        0x31f7,     /* 89 */
        0x16bf,     /* 90 */
        0xf496,     /* 91 */
        0xf9dc,     /* 92 */
        0xeadb,     /* 93 */
        0xcbc7,     /* 94 */
        0x4791,     /* 95 */
        0xb219,     /* 96 */
        0xcd2f,     /* 97 */
        0x0fee,     /* 98 */
        0xc4e8,     /* 99 */
        0x9037,     /* 100 */
        0xf6f5,     /* 101 */
        0xe7f3,     /* 102 */
        0xfc49,     /* 103 */
        0x333e,     /* 104 */
        0x3ae3,     /* 105 */
        0x7ec3,     /* 106 */
        0x73cb,     /* 107 */
        0xdf87,     /* 108 */
        0x1b87,     /* 109 */
        0x72b0,     /* 110 */
        0x8b06,     /* 111 */
        0xd0ac,     /* 112 */
        0x6d7e,     /* 113 */
        0x15f7,     /* 114 */
        0x1330,     /* 115 */
        0x22d9,     /* 116 */
        0x451a,     /* 117 */
        0xbef1,     /* 118 */
        0x4c05,     /* 119 */
        0x4e18,     /* 120 */
        0x3e33,     /* 121 */
        0x79a9,     /* 122 */
        0x10b8,     /* 123 */
        0x19b2,     /* 124 */
        0x1713,     /* 125 */
        0xc9ef,     /* 126 */
        0xcb21      /* 127 */
    },
    /* uint16 M_dash */
    0x871f
};

/* 
 * The "lump of memory passed in initialised to R^2N mod M" needed for the
 * uint16 *A parameter for the rsa_decrypt function. This must be copied into
 * a writable array of RSA_SIGNATURE_SIZE uint16 from here before use.
 */
const  uint16 rsa_decrypt_constant_sign_r2n[RSA_SIGNATURE_SIZE] = {
    0x93fd,     /* 00 */
    0x34ca,     /* 01 */
    0x717b,     /* 02 */
    0x261f,     /* 03 */
    0x3012,     /* 04 */
    0xbed0,     /* 05 */
    0xc693,     /* 06 */
    0x5c63,     /* 07 */
    0xe714,     /* 08 */
    0xe59f,     /* 09 */
    0x59c8,     /* 10 */
    0xf426,     /* 11 */
    0x79b0,     /* 12 */
    0x64e4,     /* 13 */
    0x3964,     /* 14 */
    0x614b,     /* 15 */
    0xa460,     /* 16 */
    0x0f2c,     /* 17 */
    0xa899,     /* 18 */
    0xfcd6,     /* 19 */
    0xd9a0,     /* 20 */
    0x0db7,     /* 21 */
    0xa435,     /* 22 */
    0x8aac,     /* 23 */
    0xef26,     /* 24 */
    0x40cc,     /* 25 */
    0x3a32,     /* 26 */
    0x7958,     /* 27 */
    0xe5d7,     /* 28 */
    0x60f9,     /* 29 */
    0xa1f0,     /* 30 */
    0xe9d1,     /* 31 */
    0x1468,     /* 32 */
    0x275b,     /* 33 */
    0x4058,     /* 34 */
    0xd9aa,     /* 35 */
    0x71ad,     /* 36 */
    0xda26,     /* 37 */
    0x0ad2,     /* 38 */
    0xe829,     /* 39 */
    0x3a80,     /* 40 */
    0xc177,     /* 41 */
    0xb9fd,     /* 42 */
    0x23dc,     /* 43 */
    0xf3fa,     /* 44 */
    0x91df,     /* 45 */
    0xefc8,     /* 46 */
    0x8e5d,     /* 47 */
    0x1298,     /* 48 */
    0xfe4f,     /* 49 */
    0x2e61,     /* 50 */
    0x69c1,     /* 51 */
    0x2f7c,     /* 52 */
    0xc0dd,     /* 53 */
    0x9b2c,     /* 54 */
    0x058b,     /* 55 */
    0x15ac,     /* 56 */
    0xb5cb,     /* 57 */
    0x39e8,     /* 58 */
    0x940b,     /* 59 */
    0x86f6,     /* 60 */
    0x0257,     /* 61 */
    0x8659,     /* 62 */
    0x78c2,     /* 63 */
    0xf972,     /* 64 */
    0xb605,     /* 65 */
    0x70c1,     /* 66 */
    0x130d,     /* 67 */
    0xbf79,     /* 68 */
    0xfc24,     /* 69 */
    0xe2fa,     /* 70 */
    0x66ae,     /* 71 */
    0xab04,     /* 72 */
    0xd4a2,     /* 73 */
    0x728b,     /* 74 */
    0xd2b3,     /* 75 */
    0xa1b1,     /* 76 */
    0xf233,     /* 77 */
    0x509f,     /* 78 */
    0xe7ff,     /* 79 */
    0x02c3,     /* 80 */
    0xe866,     /* 81 */
    0x3ab9,     /* 82 */
    0xce98,     /* 83 */
    0xe7b4,     /* 84 */
    0x35b8,     /* 85 */
    0xbf5a,     /* 86 */
    0xd0b0,     /* 87 */
    0x04d3,     /* 88 */
    0x23d9,     /* 89 */
    0x10cc,     /* 90 */
    0xd4d6,     /* 91 */
    0x6827,     /* 92 */
    0x5625,     /* 93 */
    0xf1e5,     /* 94 */
    0x547b,     /* 95 */
    0x5cf6,     /* 96 */
    0xd5ef,     /* 97 */
    0x8183,     /* 98 */
    0xf8a7,     /* 99 */
    0x0043,     /* 100 */
    0xfc43,     /* 101 */
    0x1c92,     /* 102 */
    0x2020,     /* 103 */
    0x3947,     /* 104 */
    0xecc7,     /* 105 */
    0x45cd,     /* 106 */
    0x0185,     /* 107 */
    0x55ff,     /* 108 */
    0x3441,     /* 109 */
    0x6951,     /* 110 */
    0xb5fa,     /* 111 */
    0x9016,     /* 112 */
    0x3438,     /* 113 */
    0x95f7,     /* 114 */
    0xb14d,     /* 115 */
    0x679d,     /* 116 */
    0x940a,     /* 117 */
    0xf1d3,     /* 118 */
    0x1c2d,     /* 119 */
    0x67e3,     /* 120 */
    0xa79f,     /* 121 */
    0x0300,     /* 122 */
    0xebd0,     /* 123 */
    0xa741,     /* 124 */
    0x3773,     /* 125 */
    0x3127,     /* 126 */
    0x277e      /* 127 */
};

/*
 * The value to be provided for the saltlen parameter to the 
 * ce_pkcs1_pss_padding_verify function used to decode the PSS padded SHA-256
 * hash. It has to match the value that was used in the encoding process.
 */
const unsigned long ce_pkcs1_pss_padding_verify_constant_saltlen = 222;
