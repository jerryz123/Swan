#include "crc32.hpp"

#include "zlib.hpp"

#include "swan.hpp"

#include "utility.hpp"

z_crc_t Coeff_64K_shift;

int crc32_config_init(size_t cache_size,
                      config_t *&config) {

    crc32_config_t *crc32_config = (crc32_config_t *)config;

    // configuration
    int length = SWAN_TXT_INPUT_SIZE;

    alloc_1D<crc32_config_t>(1, crc32_config);

    crc32_config->len = length;
    crc32_config->crc = 734276;
    crc32_config->granularity = 1;

    // in/output versions
    size_t input_size = length * sizeof(unsigned char);
    size_t output_size = 0;
    int count = cache_size / (input_size + output_size) + 1;

    config = (config_t *)crc32_config;

    return count;
}

void crc32_input_init(int count,
                      config_t *config,
                      input_t **&input) {

    crc32_config_t *crc32_config = (crc32_config_t *)config;
    crc32_input_t **crc32_input = (crc32_input_t **)input;

    // initializing input versions
    alloc_1D<crc32_input_t *>(count, crc32_input);

    // initializing individual versions
    for (int i = 0; i < count; i++) {
        alloc_1D<crc32_input_t>(1, crc32_input[i]);

        init_alloc_1D<unsigned char>(crc32_config->len, crc32_input[i]->buf);
    }

    input = (input_t **)crc32_input;
}

void crc32_output_init(int count,
                       config_t *config,
                       output_t **&output) {

    crc32_config_t *crc32_config = (crc32_config_t *)config;
    crc32_output_t **crc32_output = (crc32_output_t **)output;

    // initializing output versions
    alloc_1D<crc32_output_t *>(count, crc32_output);

    // initializing individual versions
    for (int i = 0; i < count; i++) {
        alloc_1D<crc32_output_t>(1, crc32_output[i]);

        alloc_1D<z_crc_t>(1, crc32_output[i]->return_value);
    }

    output = (output_t **)crc32_output;
}

void crc32_comparer(config_t *config,
                    output_t *output_scalar,
                    output_t *output_neon) {
    crc32_config_t *crc32_config = (crc32_config_t *)config;
    crc32_output_t *crc32_output_scalar = (crc32_output_t *)output_scalar;
    crc32_output_t *crc32_output_neon = (crc32_output_t *)output_neon;

    compare_1D<z_crc_t>(1, crc32_output_scalar->return_value, crc32_output_neon->return_value, (char *)"return_value");
}

kernel_utility_t crc32_utility = {crc32_config_init, crc32_input_init, crc32_output_init, crc32_comparer};

z_crc_t multmodp(z_crc_t a, z_crc_t b) {
    z_crc_t m, p;

    m = (z_crc_t)1 << 31;
    p = 0;
    for (;;) {
        if (a & m) {
            p ^= b;
            if ((a & (m - 1)) == 0)
                break;
        }
        m >>= 1;
        b = b & 1 ? (b >> 1) ^ POLY : b >> 1;
    }
    return p;
}

/*
  Return x^(n * 2^k) modulo p(x). Requires that x2n_table[] has been
  initialized.
 */
z_crc_t x2nmodp(int n, unsigned k) {
    z_crc_t p;

    p = (z_crc_t)1 << 31; /* x^0 == 1 */
    while (n) {
        if (n & 1)
            p = multmodp(x2n_table[k & 31], p);
        n >>= 1;
        k++;
    }
    return p;
}

z_crc_t crc32_combine64(z_crc_t crc1, z_crc_t crc2, int len2) {
    return multmodp(x2nmodp(len2, 3), crc1) ^ (crc2 & 0xffffffff);
}

/*  Return the CRC of the W bytes in the word_t data, taking the
  least-significant byte of the word as the first byte of data, without any pre
  or post conditioning. This is used to combine the CRCs of each braid.
 */
z_crc_t crc_word(z_word_t data) {
    int k;
    for (k = 0; k < W; k++)
        data = (data >> 8) ^ crc_table[data & 0xff];
    return (z_crc_t)data;
}

const z_crc_t x2n_table[] = {
    0x40000000, 0x20000000, 0x08000000, 0x00800000, 0x00008000,
    0xedb88320, 0xb1e6b092, 0xa06a2517, 0xed627dae, 0x88d14467,
    0xd7bbfe6a, 0xec447f11, 0x8e7ea170, 0x6427800e, 0x4d47bae0,
    0x09fe548f, 0x83852d0f, 0x30362f1a, 0x7b5a9cc3, 0x31fec169,
    0x9fec022a, 0x6c8dedc4, 0x15d6874d, 0x5fde7a4e, 0xbad90e37,
    0x2e4e5eef, 0x4eaba214, 0xa8a472c0, 0x429a969e, 0x148d302a,
    0xc40ba6d0, 0xc4e22c3c};

const z_crc_t crc_table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
    0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
    0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
    0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
    0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
    0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
    0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
    0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
    0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
    0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
    0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
    0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
    0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
    0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
    0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
    0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
    0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
    0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
    0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
    0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
    0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
    0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
    0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
    0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
    0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
    0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
    0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
    0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
    0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
    0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
    0x2d02ef8d};

const z_crc_t crc_braid_table[][256] = {
    {0x00000000, 0xaf449247, 0x85f822cf, 0x2abcb088, 0xd08143df,
     0x7fc5d198, 0x55796110, 0xfa3df357, 0x7a7381ff, 0xd53713b8,
     0xff8ba330, 0x50cf3177, 0xaaf2c220, 0x05b65067, 0x2f0ae0ef,
     0x804e72a8, 0xf4e703fe, 0x5ba391b9, 0x711f2131, 0xde5bb376,
     0x24664021, 0x8b22d266, 0xa19e62ee, 0x0edaf0a9, 0x8e948201,
     0x21d01046, 0x0b6ca0ce, 0xa4283289, 0x5e15c1de, 0xf1515399,
     0xdbede311, 0x74a97156, 0x32bf01bd, 0x9dfb93fa, 0xb7472372,
     0x1803b135, 0xe23e4262, 0x4d7ad025, 0x67c660ad, 0xc882f2ea,
     0x48cc8042, 0xe7881205, 0xcd34a28d, 0x627030ca, 0x984dc39d,
     0x370951da, 0x1db5e152, 0xb2f17315, 0xc6580243, 0x691c9004,
     0x43a0208c, 0xece4b2cb, 0x16d9419c, 0xb99dd3db, 0x93216353,
     0x3c65f114, 0xbc2b83bc, 0x136f11fb, 0x39d3a173, 0x96973334,
     0x6caac063, 0xc3ee5224, 0xe952e2ac, 0x461670eb, 0x657e037a,
     0xca3a913d, 0xe08621b5, 0x4fc2b3f2, 0xb5ff40a5, 0x1abbd2e2,
     0x3007626a, 0x9f43f02d, 0x1f0d8285, 0xb04910c2, 0x9af5a04a,
     0x35b1320d, 0xcf8cc15a, 0x60c8531d, 0x4a74e395, 0xe53071d2,
     0x91990084, 0x3edd92c3, 0x1461224b, 0xbb25b00c, 0x4118435b,
     0xee5cd11c, 0xc4e06194, 0x6ba4f3d3, 0xebea817b, 0x44ae133c,
     0x6e12a3b4, 0xc15631f3, 0x3b6bc2a4, 0x942f50e3, 0xbe93e06b,
     0x11d7722c, 0x57c102c7, 0xf8859080, 0xd2392008, 0x7d7db24f,
     0x87404118, 0x2804d35f, 0x02b863d7, 0xadfcf190, 0x2db28338,
     0x82f6117f, 0xa84aa1f7, 0x070e33b0, 0xfd33c0e7, 0x527752a0,
     0x78cbe228, 0xd78f706f, 0xa3260139, 0x0c62937e, 0x26de23f6,
     0x899ab1b1, 0x73a742e6, 0xdce3d0a1, 0xf65f6029, 0x591bf26e,
     0xd95580c6, 0x76111281, 0x5cada209, 0xf3e9304e, 0x09d4c319,
     0xa690515e, 0x8c2ce1d6, 0x23687391, 0xcafc06f4, 0x65b894b3,
     0x4f04243b, 0xe040b67c, 0x1a7d452b, 0xb539d76c, 0x9f8567e4,
     0x30c1f5a3, 0xb08f870b, 0x1fcb154c, 0x3577a5c4, 0x9a333783,
     0x600ec4d4, 0xcf4a5693, 0xe5f6e61b, 0x4ab2745c, 0x3e1b050a,
     0x915f974d, 0xbbe327c5, 0x14a7b582, 0xee9a46d5, 0x41ded492,
     0x6b62641a, 0xc426f65d, 0x446884f5, 0xeb2c16b2, 0xc190a63a,
     0x6ed4347d, 0x94e9c72a, 0x3bad556d, 0x1111e5e5, 0xbe5577a2,
     0xf8430749, 0x5707950e, 0x7dbb2586, 0xd2ffb7c1, 0x28c24496,
     0x8786d6d1, 0xad3a6659, 0x027ef41e, 0x823086b6, 0x2d7414f1,
     0x07c8a479, 0xa88c363e, 0x52b1c569, 0xfdf5572e, 0xd749e7a6,
     0x780d75e1, 0x0ca404b7, 0xa3e096f0, 0x895c2678, 0x2618b43f,
     0xdc254768, 0x7361d52f, 0x59dd65a7, 0xf699f7e0, 0x76d78548,
     0xd993170f, 0xf32fa787, 0x5c6b35c0, 0xa656c697, 0x091254d0,
     0x23aee458, 0x8cea761f, 0xaf82058e, 0x00c697c9, 0x2a7a2741,
     0x853eb506, 0x7f034651, 0xd047d416, 0xfafb649e, 0x55bff6d9,
     0xd5f18471, 0x7ab51636, 0x5009a6be, 0xff4d34f9, 0x0570c7ae,
     0xaa3455e9, 0x8088e561, 0x2fcc7726, 0x5b650670, 0xf4219437,
     0xde9d24bf, 0x71d9b6f8, 0x8be445af, 0x24a0d7e8, 0x0e1c6760,
     0xa158f527, 0x2116878f, 0x8e5215c8, 0xa4eea540, 0x0baa3707,
     0xf197c450, 0x5ed35617, 0x746fe69f, 0xdb2b74d8, 0x9d3d0433,
     0x32799674, 0x18c526fc, 0xb781b4bb, 0x4dbc47ec, 0xe2f8d5ab,
     0xc8446523, 0x6700f764, 0xe74e85cc, 0x480a178b, 0x62b6a703,
     0xcdf23544, 0x37cfc613, 0x988b5454, 0xb237e4dc, 0x1d73769b,
     0x69da07cd, 0xc69e958a, 0xec222502, 0x4366b745, 0xb95b4412,
     0x161fd655, 0x3ca366dd, 0x93e7f49a, 0x13a98632, 0xbced1475,
     0x9651a4fd, 0x391536ba, 0xc328c5ed, 0x6c6c57aa, 0x46d0e722,
     0xe9947565},
    {0x00000000, 0x4e890ba9, 0x9d121752, 0xd39b1cfb, 0xe15528e5,
     0xafdc234c, 0x7c473fb7, 0x32ce341e, 0x19db578b, 0x57525c22,
     0x84c940d9, 0xca404b70, 0xf88e7f6e, 0xb60774c7, 0x659c683c,
     0x2b156395, 0x33b6af16, 0x7d3fa4bf, 0xaea4b844, 0xe02db3ed,
     0xd2e387f3, 0x9c6a8c5a, 0x4ff190a1, 0x01789b08, 0x2a6df89d,
     0x64e4f334, 0xb77fefcf, 0xf9f6e466, 0xcb38d078, 0x85b1dbd1,
     0x562ac72a, 0x18a3cc83, 0x676d5e2c, 0x29e45585, 0xfa7f497e,
     0xb4f642d7, 0x863876c9, 0xc8b17d60, 0x1b2a619b, 0x55a36a32,
     0x7eb609a7, 0x303f020e, 0xe3a41ef5, 0xad2d155c, 0x9fe32142,
     0xd16a2aeb, 0x02f13610, 0x4c783db9, 0x54dbf13a, 0x1a52fa93,
     0xc9c9e668, 0x8740edc1, 0xb58ed9df, 0xfb07d276, 0x289cce8d,
     0x6615c524, 0x4d00a6b1, 0x0389ad18, 0xd012b1e3, 0x9e9bba4a,
     0xac558e54, 0xe2dc85fd, 0x31479906, 0x7fce92af, 0xcedabc58,
     0x8053b7f1, 0x53c8ab0a, 0x1d41a0a3, 0x2f8f94bd, 0x61069f14,
     0xb29d83ef, 0xfc148846, 0xd701ebd3, 0x9988e07a, 0x4a13fc81,
     0x049af728, 0x3654c336, 0x78ddc89f, 0xab46d464, 0xe5cfdfcd,
     0xfd6c134e, 0xb3e518e7, 0x607e041c, 0x2ef70fb5, 0x1c393bab,
     0x52b03002, 0x812b2cf9, 0xcfa22750, 0xe4b744c5, 0xaa3e4f6c,
     0x79a55397, 0x372c583e, 0x05e26c20, 0x4b6b6789, 0x98f07b72,
     0xd67970db, 0xa9b7e274, 0xe73ee9dd, 0x34a5f526, 0x7a2cfe8f,
     0x48e2ca91, 0x066bc138, 0xd5f0ddc3, 0x9b79d66a, 0xb06cb5ff,
     0xfee5be56, 0x2d7ea2ad, 0x63f7a904, 0x51399d1a, 0x1fb096b3,
     0xcc2b8a48, 0x82a281e1, 0x9a014d62, 0xd48846cb, 0x07135a30,
     0x499a5199, 0x7b546587, 0x35dd6e2e, 0xe64672d5, 0xa8cf797c,
     0x83da1ae9, 0xcd531140, 0x1ec80dbb, 0x50410612, 0x628f320c,
     0x2c0639a5, 0xff9d255e, 0xb1142ef7, 0x46c47ef1, 0x084d7558,
     0xdbd669a3, 0x955f620a, 0xa7915614, 0xe9185dbd, 0x3a834146,
     0x740a4aef, 0x5f1f297a, 0x119622d3, 0xc20d3e28, 0x8c843581,
     0xbe4a019f, 0xf0c30a36, 0x235816cd, 0x6dd11d64, 0x7572d1e7,
     0x3bfbda4e, 0xe860c6b5, 0xa6e9cd1c, 0x9427f902, 0xdaaef2ab,
     0x0935ee50, 0x47bce5f9, 0x6ca9866c, 0x22208dc5, 0xf1bb913e,
     0xbf329a97, 0x8dfcae89, 0xc375a520, 0x10eeb9db, 0x5e67b272,
     0x21a920dd, 0x6f202b74, 0xbcbb378f, 0xf2323c26, 0xc0fc0838,
     0x8e750391, 0x5dee1f6a, 0x136714c3, 0x38727756, 0x76fb7cff,
     0xa5606004, 0xebe96bad, 0xd9275fb3, 0x97ae541a, 0x443548e1,
     0x0abc4348, 0x121f8fcb, 0x5c968462, 0x8f0d9899, 0xc1849330,
     0xf34aa72e, 0xbdc3ac87, 0x6e58b07c, 0x20d1bbd5, 0x0bc4d840,
     0x454dd3e9, 0x96d6cf12, 0xd85fc4bb, 0xea91f0a5, 0xa418fb0c,
     0x7783e7f7, 0x390aec5e, 0x881ec2a9, 0xc697c900, 0x150cd5fb,
     0x5b85de52, 0x694bea4c, 0x27c2e1e5, 0xf459fd1e, 0xbad0f6b7,
     0x91c59522, 0xdf4c9e8b, 0x0cd78270, 0x425e89d9, 0x7090bdc7,
     0x3e19b66e, 0xed82aa95, 0xa30ba13c, 0xbba86dbf, 0xf5216616,
     0x26ba7aed, 0x68337144, 0x5afd455a, 0x14744ef3, 0xc7ef5208,
     0x896659a1, 0xa2733a34, 0xecfa319d, 0x3f612d66, 0x71e826cf,
     0x432612d1, 0x0daf1978, 0xde340583, 0x90bd0e2a, 0xef739c85,
     0xa1fa972c, 0x72618bd7, 0x3ce8807e, 0x0e26b460, 0x40afbfc9,
     0x9334a332, 0xddbda89b, 0xf6a8cb0e, 0xb821c0a7, 0x6bbadc5c,
     0x2533d7f5, 0x17fde3eb, 0x5974e842, 0x8aeff4b9, 0xc466ff10,
     0xdcc53393, 0x924c383a, 0x41d724c1, 0x0f5e2f68, 0x3d901b76,
     0x731910df, 0xa0820c24, 0xee0b078d, 0xc51e6418, 0x8b976fb1,
     0x580c734a, 0x168578e3, 0x244b4cfd, 0x6ac24754, 0xb9595baf,
     0xf7d05006},
    {0x00000000, 0x8d88fde2, 0xc060fd85, 0x4de80067, 0x5bb0fd4b,
     0xd63800a9, 0x9bd000ce, 0x1658fd2c, 0xb761fa96, 0x3ae90774,
     0x77010713, 0xfa89faf1, 0xecd107dd, 0x6159fa3f, 0x2cb1fa58,
     0xa13907ba, 0xb5b2f36d, 0x383a0e8f, 0x75d20ee8, 0xf85af30a,
     0xee020e26, 0x638af3c4, 0x2e62f3a3, 0xa3ea0e41, 0x02d309fb,
     0x8f5bf419, 0xc2b3f47e, 0x4f3b099c, 0x5963f4b0, 0xd4eb0952,
     0x99030935, 0x148bf4d7, 0xb014e09b, 0x3d9c1d79, 0x70741d1e,
     0xfdfce0fc, 0xeba41dd0, 0x662ce032, 0x2bc4e055, 0xa64c1db7,
     0x07751a0d, 0x8afde7ef, 0xc715e788, 0x4a9d1a6a, 0x5cc5e746,
     0xd14d1aa4, 0x9ca51ac3, 0x112de721, 0x05a613f6, 0x882eee14,
     0xc5c6ee73, 0x484e1391, 0x5e16eebd, 0xd39e135f, 0x9e761338,
     0x13feeeda, 0xb2c7e960, 0x3f4f1482, 0x72a714e5, 0xff2fe907,
     0xe977142b, 0x64ffe9c9, 0x2917e9ae, 0xa49f144c, 0xbb58c777,
     0x36d03a95, 0x7b383af2, 0xf6b0c710, 0xe0e83a3c, 0x6d60c7de,
     0x2088c7b9, 0xad003a5b, 0x0c393de1, 0x81b1c003, 0xcc59c064,
     0x41d13d86, 0x5789c0aa, 0xda013d48, 0x97e93d2f, 0x1a61c0cd,
     0x0eea341a, 0x8362c9f8, 0xce8ac99f, 0x4302347d, 0x555ac951,
     0xd8d234b3, 0x953a34d4, 0x18b2c936, 0xb98bce8c, 0x3403336e,
     0x79eb3309, 0xf463ceeb, 0xe23b33c7, 0x6fb3ce25, 0x225bce42,
     0xafd333a0, 0x0b4c27ec, 0x86c4da0e, 0xcb2cda69, 0x46a4278b,
     0x50fcdaa7, 0xdd742745, 0x909c2722, 0x1d14dac0, 0xbc2ddd7a,
     0x31a52098, 0x7c4d20ff, 0xf1c5dd1d, 0xe79d2031, 0x6a15ddd3,
     0x27fdddb4, 0xaa752056, 0xbefed481, 0x33762963, 0x7e9e2904,
     0xf316d4e6, 0xe54e29ca, 0x68c6d428, 0x252ed44f, 0xa8a629ad,
     0x099f2e17, 0x8417d3f5, 0xc9ffd392, 0x44772e70, 0x522fd35c,
     0xdfa72ebe, 0x924f2ed9, 0x1fc7d33b, 0xadc088af, 0x2048754d,
     0x6da0752a, 0xe02888c8, 0xf67075e4, 0x7bf88806, 0x36108861,
     0xbb987583, 0x1aa17239, 0x97298fdb, 0xdac18fbc, 0x5749725e,
     0x41118f72, 0xcc997290, 0x817172f7, 0x0cf98f15, 0x18727bc2,
     0x95fa8620, 0xd8128647, 0x559a7ba5, 0x43c28689, 0xce4a7b6b,
     0x83a27b0c, 0x0e2a86ee, 0xaf138154, 0x229b7cb6, 0x6f737cd1,
     0xe2fb8133, 0xf4a37c1f, 0x792b81fd, 0x34c3819a, 0xb94b7c78,
     0x1dd46834, 0x905c95d6, 0xddb495b1, 0x503c6853, 0x4664957f,
     0xcbec689d, 0x860468fa, 0x0b8c9518, 0xaab592a2, 0x273d6f40,
     0x6ad56f27, 0xe75d92c5, 0xf1056fe9, 0x7c8d920b, 0x3165926c,
     0xbced6f8e, 0xa8669b59, 0x25ee66bb, 0x680666dc, 0xe58e9b3e,
     0xf3d66612, 0x7e5e9bf0, 0x33b69b97, 0xbe3e6675, 0x1f0761cf,
     0x928f9c2d, 0xdf679c4a, 0x52ef61a8, 0x44b79c84, 0xc93f6166,
     0x84d76101, 0x095f9ce3, 0x16984fd8, 0x9b10b23a, 0xd6f8b25d,
     0x5b704fbf, 0x4d28b293, 0xc0a04f71, 0x8d484f16, 0x00c0b2f4,
     0xa1f9b54e, 0x2c7148ac, 0x619948cb, 0xec11b529, 0xfa494805,
     0x77c1b5e7, 0x3a29b580, 0xb7a14862, 0xa32abcb5, 0x2ea24157,
     0x634a4130, 0xeec2bcd2, 0xf89a41fe, 0x7512bc1c, 0x38fabc7b,
     0xb5724199, 0x144b4623, 0x99c3bbc1, 0xd42bbba6, 0x59a34644,
     0x4ffbbb68, 0xc273468a, 0x8f9b46ed, 0x0213bb0f, 0xa68caf43,
     0x2b0452a1, 0x66ec52c6, 0xeb64af24, 0xfd3c5208, 0x70b4afea,
     0x3d5caf8d, 0xb0d4526f, 0x11ed55d5, 0x9c65a837, 0xd18da850,
     0x5c0555b2, 0x4a5da89e, 0xc7d5557c, 0x8a3d551b, 0x07b5a8f9,
     0x133e5c2e, 0x9eb6a1cc, 0xd35ea1ab, 0x5ed65c49, 0x488ea165,
     0xc5065c87, 0x88ee5ce0, 0x0566a102, 0xa45fa6b8, 0x29d75b5a,
     0x643f5b3d, 0xe9b7a6df, 0xffef5bf3, 0x7267a611, 0x3f8fa676,
     0xb2075b94},
    {0x00000000, 0x80f0171f, 0xda91287f, 0x5a613f60, 0x6e5356bf,
     0xeea341a0, 0xb4c27ec0, 0x343269df, 0xdca6ad7e, 0x5c56ba61,
     0x06378501, 0x86c7921e, 0xb2f5fbc1, 0x3205ecde, 0x6864d3be,
     0xe894c4a1, 0x623c5cbd, 0xe2cc4ba2, 0xb8ad74c2, 0x385d63dd,
     0x0c6f0a02, 0x8c9f1d1d, 0xd6fe227d, 0x560e3562, 0xbe9af1c3,
     0x3e6ae6dc, 0x640bd9bc, 0xe4fbcea3, 0xd0c9a77c, 0x5039b063,
     0x0a588f03, 0x8aa8981c, 0xc478b97a, 0x4488ae65, 0x1ee99105,
     0x9e19861a, 0xaa2befc5, 0x2adbf8da, 0x70bac7ba, 0xf04ad0a5,
     0x18de1404, 0x982e031b, 0xc24f3c7b, 0x42bf2b64, 0x768d42bb,
     0xf67d55a4, 0xac1c6ac4, 0x2cec7ddb, 0xa644e5c7, 0x26b4f2d8,
     0x7cd5cdb8, 0xfc25daa7, 0xc817b378, 0x48e7a467, 0x12869b07,
     0x92768c18, 0x7ae248b9, 0xfa125fa6, 0xa07360c6, 0x208377d9,
     0x14b11e06, 0x94410919, 0xce203679, 0x4ed02166, 0x538074b5,
     0xd37063aa, 0x89115cca, 0x09e14bd5, 0x3dd3220a, 0xbd233515,
     0xe7420a75, 0x67b21d6a, 0x8f26d9cb, 0x0fd6ced4, 0x55b7f1b4,
     0xd547e6ab, 0xe1758f74, 0x6185986b, 0x3be4a70b, 0xbb14b014,
     0x31bc2808, 0xb14c3f17, 0xeb2d0077, 0x6bdd1768, 0x5fef7eb7,
     0xdf1f69a8, 0x857e56c8, 0x058e41d7, 0xed1a8576, 0x6dea9269,
     0x378bad09, 0xb77bba16, 0x8349d3c9, 0x03b9c4d6, 0x59d8fbb6,
     0xd928eca9, 0x97f8cdcf, 0x1708dad0, 0x4d69e5b0, 0xcd99f2af,
     0xf9ab9b70, 0x795b8c6f, 0x233ab30f, 0xa3caa410, 0x4b5e60b1,
     0xcbae77ae, 0x91cf48ce, 0x113f5fd1, 0x250d360e, 0xa5fd2111,
     0xff9c1e71, 0x7f6c096e, 0xf5c49172, 0x7534866d, 0x2f55b90d,
     0xafa5ae12, 0x9b97c7cd, 0x1b67d0d2, 0x4106efb2, 0xc1f6f8ad,
     0x29623c0c, 0xa9922b13, 0xf3f31473, 0x7303036c, 0x47316ab3,
     0xc7c17dac, 0x9da042cc, 0x1d5055d3, 0xa700e96a, 0x27f0fe75,
     0x7d91c115, 0xfd61d60a, 0xc953bfd5, 0x49a3a8ca, 0x13c297aa,
     0x933280b5, 0x7ba64414, 0xfb56530b, 0xa1376c6b, 0x21c77b74,
     0x15f512ab, 0x950505b4, 0xcf643ad4, 0x4f942dcb, 0xc53cb5d7,
     0x45cca2c8, 0x1fad9da8, 0x9f5d8ab7, 0xab6fe368, 0x2b9ff477,
     0x71fecb17, 0xf10edc08, 0x199a18a9, 0x996a0fb6, 0xc30b30d6,
     0x43fb27c9, 0x77c94e16, 0xf7395909, 0xad586669, 0x2da87176,
     0x63785010, 0xe388470f, 0xb9e9786f, 0x39196f70, 0x0d2b06af,
     0x8ddb11b0, 0xd7ba2ed0, 0x574a39cf, 0xbfdefd6e, 0x3f2eea71,
     0x654fd511, 0xe5bfc20e, 0xd18dabd1, 0x517dbcce, 0x0b1c83ae,
     0x8bec94b1, 0x01440cad, 0x81b41bb2, 0xdbd524d2, 0x5b2533cd,
     0x6f175a12, 0xefe74d0d, 0xb586726d, 0x35766572, 0xdde2a1d3,
     0x5d12b6cc, 0x077389ac, 0x87839eb3, 0xb3b1f76c, 0x3341e073,
     0x6920df13, 0xe9d0c80c, 0xf4809ddf, 0x74708ac0, 0x2e11b5a0,
     0xaee1a2bf, 0x9ad3cb60, 0x1a23dc7f, 0x4042e31f, 0xc0b2f400,
     0x282630a1, 0xa8d627be, 0xf2b718de, 0x72470fc1, 0x4675661e,
     0xc6857101, 0x9ce44e61, 0x1c14597e, 0x96bcc162, 0x164cd67d,
     0x4c2de91d, 0xccddfe02, 0xf8ef97dd, 0x781f80c2, 0x227ebfa2,
     0xa28ea8bd, 0x4a1a6c1c, 0xcaea7b03, 0x908b4463, 0x107b537c,
     0x24493aa3, 0xa4b92dbc, 0xfed812dc, 0x7e2805c3, 0x30f824a5,
     0xb00833ba, 0xea690cda, 0x6a991bc5, 0x5eab721a, 0xde5b6505,
     0x843a5a65, 0x04ca4d7a, 0xec5e89db, 0x6cae9ec4, 0x36cfa1a4,
     0xb63fb6bb, 0x820ddf64, 0x02fdc87b, 0x589cf71b, 0xd86ce004,
     0x52c47818, 0xd2346f07, 0x88555067, 0x08a54778, 0x3c972ea7,
     0xbc6739b8, 0xe60606d8, 0x66f611c7, 0x8e62d566, 0x0e92c279,
     0x54f3fd19, 0xd403ea06, 0xe03183d9, 0x60c194c6, 0x3aa0aba6,
     0xba50bcb9},
    {0x00000000, 0x9570d495, 0xf190af6b, 0x64e07bfe, 0x38505897,
     0xad208c02, 0xc9c0f7fc, 0x5cb02369, 0x70a0b12e, 0xe5d065bb,
     0x81301e45, 0x1440cad0, 0x48f0e9b9, 0xdd803d2c, 0xb96046d2,
     0x2c109247, 0xe141625c, 0x7431b6c9, 0x10d1cd37, 0x85a119a2,
     0xd9113acb, 0x4c61ee5e, 0x288195a0, 0xbdf14135, 0x91e1d372,
     0x049107e7, 0x60717c19, 0xf501a88c, 0xa9b18be5, 0x3cc15f70,
     0x5821248e, 0xcd51f01b, 0x19f3c2f9, 0x8c83166c, 0xe8636d92,
     0x7d13b907, 0x21a39a6e, 0xb4d34efb, 0xd0333505, 0x4543e190,
     0x695373d7, 0xfc23a742, 0x98c3dcbc, 0x0db30829, 0x51032b40,
     0xc473ffd5, 0xa093842b, 0x35e350be, 0xf8b2a0a5, 0x6dc27430,
     0x09220fce, 0x9c52db5b, 0xc0e2f832, 0x55922ca7, 0x31725759,
     0xa40283cc, 0x8812118b, 0x1d62c51e, 0x7982bee0, 0xecf26a75,
     0xb042491c, 0x25329d89, 0x41d2e677, 0xd4a232e2, 0x33e785f2,
     0xa6975167, 0xc2772a99, 0x5707fe0c, 0x0bb7dd65, 0x9ec709f0,
     0xfa27720e, 0x6f57a69b, 0x434734dc, 0xd637e049, 0xb2d79bb7,
     0x27a74f22, 0x7b176c4b, 0xee67b8de, 0x8a87c320, 0x1ff717b5,
     0xd2a6e7ae, 0x47d6333b, 0x233648c5, 0xb6469c50, 0xeaf6bf39,
     0x7f866bac, 0x1b661052, 0x8e16c4c7, 0xa2065680, 0x37768215,
     0x5396f9eb, 0xc6e62d7e, 0x9a560e17, 0x0f26da82, 0x6bc6a17c,
     0xfeb675e9, 0x2a14470b, 0xbf64939e, 0xdb84e860, 0x4ef43cf5,
     0x12441f9c, 0x8734cb09, 0xe3d4b0f7, 0x76a46462, 0x5ab4f625,
     0xcfc422b0, 0xab24594e, 0x3e548ddb, 0x62e4aeb2, 0xf7947a27,
     0x937401d9, 0x0604d54c, 0xcb552557, 0x5e25f1c2, 0x3ac58a3c,
     0xafb55ea9, 0xf3057dc0, 0x6675a955, 0x0295d2ab, 0x97e5063e,
     0xbbf59479, 0x2e8540ec, 0x4a653b12, 0xdf15ef87, 0x83a5ccee,
     0x16d5187b, 0x72356385, 0xe745b710, 0x67cf0be4, 0xf2bfdf71,
     0x965fa48f, 0x032f701a, 0x5f9f5373, 0xcaef87e6, 0xae0ffc18,
     0x3b7f288d, 0x176fbaca, 0x821f6e5f, 0xe6ff15a1, 0x738fc134,
     0x2f3fe25d, 0xba4f36c8, 0xdeaf4d36, 0x4bdf99a3, 0x868e69b8,
     0x13febd2d, 0x771ec6d3, 0xe26e1246, 0xbede312f, 0x2baee5ba,
     0x4f4e9e44, 0xda3e4ad1, 0xf62ed896, 0x635e0c03, 0x07be77fd,
     0x92cea368, 0xce7e8001, 0x5b0e5494, 0x3fee2f6a, 0xaa9efbff,
     0x7e3cc91d, 0xeb4c1d88, 0x8fac6676, 0x1adcb2e3, 0x466c918a,
     0xd31c451f, 0xb7fc3ee1, 0x228cea74, 0x0e9c7833, 0x9becaca6,
     0xff0cd758, 0x6a7c03cd, 0x36cc20a4, 0xa3bcf431, 0xc75c8fcf,
     0x522c5b5a, 0x9f7dab41, 0x0a0d7fd4, 0x6eed042a, 0xfb9dd0bf,
     0xa72df3d6, 0x325d2743, 0x56bd5cbd, 0xc3cd8828, 0xefdd1a6f,
     0x7aadcefa, 0x1e4db504, 0x8b3d6191, 0xd78d42f8, 0x42fd966d,
     0x261ded93, 0xb36d3906, 0x54288e16, 0xc1585a83, 0xa5b8217d,
     0x30c8f5e8, 0x6c78d681, 0xf9080214, 0x9de879ea, 0x0898ad7f,
     0x24883f38, 0xb1f8ebad, 0xd5189053, 0x406844c6, 0x1cd867af,
     0x89a8b33a, 0xed48c8c4, 0x78381c51, 0xb569ec4a, 0x201938df,
     0x44f94321, 0xd18997b4, 0x8d39b4dd, 0x18496048, 0x7ca91bb6,
     0xe9d9cf23, 0xc5c95d64, 0x50b989f1, 0x3459f20f, 0xa129269a,
     0xfd9905f3, 0x68e9d166, 0x0c09aa98, 0x99797e0d, 0x4ddb4cef,
     0xd8ab987a, 0xbc4be384, 0x293b3711, 0x758b1478, 0xe0fbc0ed,
     0x841bbb13, 0x116b6f86, 0x3d7bfdc1, 0xa80b2954, 0xcceb52aa,
     0x599b863f, 0x052ba556, 0x905b71c3, 0xf4bb0a3d, 0x61cbdea8,
     0xac9a2eb3, 0x39eafa26, 0x5d0a81d8, 0xc87a554d, 0x94ca7624,
     0x01baa2b1, 0x655ad94f, 0xf02a0dda, 0xdc3a9f9d, 0x494a4b08,
     0x2daa30f6, 0xb8dae463, 0xe46ac70a, 0x711a139f, 0x15fa6861,
     0x808abcf4},
    {0x00000000, 0xcf9e17c8, 0x444d29d1, 0x8bd33e19, 0x889a53a2,
     0x4704446a, 0xccd77a73, 0x03496dbb, 0xca45a105, 0x05dbb6cd,
     0x8e0888d4, 0x41969f1c, 0x42dff2a7, 0x8d41e56f, 0x0692db76,
     0xc90cccbe, 0x4ffa444b, 0x80645383, 0x0bb76d9a, 0xc4297a52,
     0xc76017e9, 0x08fe0021, 0x832d3e38, 0x4cb329f0, 0x85bfe54e,
     0x4a21f286, 0xc1f2cc9f, 0x0e6cdb57, 0x0d25b6ec, 0xc2bba124,
     0x49689f3d, 0x86f688f5, 0x9ff48896, 0x506a9f5e, 0xdbb9a147,
     0x1427b68f, 0x176edb34, 0xd8f0ccfc, 0x5323f2e5, 0x9cbde52d,
     0x55b12993, 0x9a2f3e5b, 0x11fc0042, 0xde62178a, 0xdd2b7a31,
     0x12b56df9, 0x996653e0, 0x56f84428, 0xd00eccdd, 0x1f90db15,
     0x9443e50c, 0x5bddf2c4, 0x58949f7f, 0x970a88b7, 0x1cd9b6ae,
     0xd347a166, 0x1a4b6dd8, 0xd5d57a10, 0x5e064409, 0x919853c1,
     0x92d13e7a, 0x5d4f29b2, 0xd69c17ab, 0x19020063, 0xe498176d,
     0x2b0600a5, 0xa0d53ebc, 0x6f4b2974, 0x6c0244cf, 0xa39c5307,
     0x284f6d1e, 0xe7d17ad6, 0x2eddb668, 0xe143a1a0, 0x6a909fb9,
     0xa50e8871, 0xa647e5ca, 0x69d9f202, 0xe20acc1b, 0x2d94dbd3,
     0xab625326, 0x64fc44ee, 0xef2f7af7, 0x20b16d3f, 0x23f80084,
     0xec66174c, 0x67b52955, 0xa82b3e9d, 0x6127f223, 0xaeb9e5eb,
     0x256adbf2, 0xeaf4cc3a, 0xe9bda181, 0x2623b649, 0xadf08850,
     0x626e9f98, 0x7b6c9ffb, 0xb4f28833, 0x3f21b62a, 0xf0bfa1e2,
     0xf3f6cc59, 0x3c68db91, 0xb7bbe588, 0x7825f240, 0xb1293efe,
     0x7eb72936, 0xf564172f, 0x3afa00e7, 0x39b36d5c, 0xf62d7a94,
     0x7dfe448d, 0xb2605345, 0x3496dbb0, 0xfb08cc78, 0x70dbf261,
     0xbf45e5a9, 0xbc0c8812, 0x73929fda, 0xf841a1c3, 0x37dfb60b,
     0xfed37ab5, 0x314d6d7d, 0xba9e5364, 0x750044ac, 0x76492917,
     0xb9d73edf, 0x320400c6, 0xfd9a170e, 0x1241289b, 0xdddf3f53,
     0x560c014a, 0x99921682, 0x9adb7b39, 0x55456cf1, 0xde9652e8,
     0x11084520, 0xd804899e, 0x179a9e56, 0x9c49a04f, 0x53d7b787,
     0x509eda3c, 0x9f00cdf4, 0x14d3f3ed, 0xdb4de425, 0x5dbb6cd0,
     0x92257b18, 0x19f64501, 0xd66852c9, 0xd5213f72, 0x1abf28ba,
     0x916c16a3, 0x5ef2016b, 0x97fecdd5, 0x5860da1d, 0xd3b3e404,
     0x1c2df3cc, 0x1f649e77, 0xd0fa89bf, 0x5b29b7a6, 0x94b7a06e,
     0x8db5a00d, 0x422bb7c5, 0xc9f889dc, 0x06669e14, 0x052ff3af,
     0xcab1e467, 0x4162da7e, 0x8efccdb6, 0x47f00108, 0x886e16c0,
     0x03bd28d9, 0xcc233f11, 0xcf6a52aa, 0x00f44562, 0x8b277b7b,
     0x44b96cb3, 0xc24fe446, 0x0dd1f38e, 0x8602cd97, 0x499cda5f,
     0x4ad5b7e4, 0x854ba02c, 0x0e989e35, 0xc10689fd, 0x080a4543,
     0xc794528b, 0x4c476c92, 0x83d97b5a, 0x809016e1, 0x4f0e0129,
     0xc4dd3f30, 0x0b4328f8, 0xf6d93ff6, 0x3947283e, 0xb2941627,
     0x7d0a01ef, 0x7e436c54, 0xb1dd7b9c, 0x3a0e4585, 0xf590524d,
     0x3c9c9ef3, 0xf302893b, 0x78d1b722, 0xb74fa0ea, 0xb406cd51,
     0x7b98da99, 0xf04be480, 0x3fd5f348, 0xb9237bbd, 0x76bd6c75,
     0xfd6e526c, 0x32f045a4, 0x31b9281f, 0xfe273fd7, 0x75f401ce,
     0xba6a1606, 0x7366dab8, 0xbcf8cd70, 0x372bf369, 0xf8b5e4a1,
     0xfbfc891a, 0x34629ed2, 0xbfb1a0cb, 0x702fb703, 0x692db760,
     0xa6b3a0a8, 0x2d609eb1, 0xe2fe8979, 0xe1b7e4c2, 0x2e29f30a,
     0xa5facd13, 0x6a64dadb, 0xa3681665, 0x6cf601ad, 0xe7253fb4,
     0x28bb287c, 0x2bf245c7, 0xe46c520f, 0x6fbf6c16, 0xa0217bde,
     0x26d7f32b, 0xe949e4e3, 0x629adafa, 0xad04cd32, 0xae4da089,
     0x61d3b741, 0xea008958, 0x259e9e90, 0xec92522e, 0x230c45e6,
     0xa8df7bff, 0x67416c37, 0x6408018c, 0xab961644, 0x2045285d,
     0xefdb3f95},
    {0x00000000, 0x24825136, 0x4904a26c, 0x6d86f35a, 0x920944d8,
     0xb68b15ee, 0xdb0de6b4, 0xff8fb782, 0xff638ff1, 0xdbe1dec7,
     0xb6672d9d, 0x92e57cab, 0x6d6acb29, 0x49e89a1f, 0x246e6945,
     0x00ec3873, 0x25b619a3, 0x01344895, 0x6cb2bbcf, 0x4830eaf9,
     0xb7bf5d7b, 0x933d0c4d, 0xfebbff17, 0xda39ae21, 0xdad59652,
     0xfe57c764, 0x93d1343e, 0xb7536508, 0x48dcd28a, 0x6c5e83bc,
     0x01d870e6, 0x255a21d0, 0x4b6c3346, 0x6fee6270, 0x0268912a,
     0x26eac01c, 0xd965779e, 0xfde726a8, 0x9061d5f2, 0xb4e384c4,
     0xb40fbcb7, 0x908ded81, 0xfd0b1edb, 0xd9894fed, 0x2606f86f,
     0x0284a959, 0x6f025a03, 0x4b800b35, 0x6eda2ae5, 0x4a587bd3,
     0x27de8889, 0x035cd9bf, 0xfcd36e3d, 0xd8513f0b, 0xb5d7cc51,
     0x91559d67, 0x91b9a514, 0xb53bf422, 0xd8bd0778, 0xfc3f564e,
     0x03b0e1cc, 0x2732b0fa, 0x4ab443a0, 0x6e361296, 0x96d8668c,
     0xb25a37ba, 0xdfdcc4e0, 0xfb5e95d6, 0x04d12254, 0x20537362,
     0x4dd58038, 0x6957d10e, 0x69bbe97d, 0x4d39b84b, 0x20bf4b11,
     0x043d1a27, 0xfbb2ada5, 0xdf30fc93, 0xb2b60fc9, 0x96345eff,
     0xb36e7f2f, 0x97ec2e19, 0xfa6add43, 0xdee88c75, 0x21673bf7,
     0x05e56ac1, 0x6863999b, 0x4ce1c8ad, 0x4c0df0de, 0x688fa1e8,
     0x050952b2, 0x218b0384, 0xde04b406, 0xfa86e530, 0x9700166a,
     0xb382475c, 0xddb455ca, 0xf93604fc, 0x94b0f7a6, 0xb032a690,
     0x4fbd1112, 0x6b3f4024, 0x06b9b37e, 0x223be248, 0x22d7da3b,
     0x06558b0d, 0x6bd37857, 0x4f512961, 0xb0de9ee3, 0x945ccfd5,
     0xf9da3c8f, 0xdd586db9, 0xf8024c69, 0xdc801d5f, 0xb106ee05,
     0x9584bf33, 0x6a0b08b1, 0x4e895987, 0x230faadd, 0x078dfbeb,
     0x0761c398, 0x23e392ae, 0x4e6561f4, 0x6ae730c2, 0x95688740,
     0xb1ead676, 0xdc6c252c, 0xf8ee741a, 0xf6c1cb59, 0xd2439a6f,
     0xbfc56935, 0x9b473803, 0x64c88f81, 0x404adeb7, 0x2dcc2ded,
     0x094e7cdb, 0x09a244a8, 0x2d20159e, 0x40a6e6c4, 0x6424b7f2,
     0x9bab0070, 0xbf295146, 0xd2afa21c, 0xf62df32a, 0xd377d2fa,
     0xf7f583cc, 0x9a737096, 0xbef121a0, 0x417e9622, 0x65fcc714,
     0x087a344e, 0x2cf86578, 0x2c145d0b, 0x08960c3d, 0x6510ff67,
     0x4192ae51, 0xbe1d19d3, 0x9a9f48e5, 0xf719bbbf, 0xd39bea89,
     0xbdadf81f, 0x992fa929, 0xf4a95a73, 0xd02b0b45, 0x2fa4bcc7,
     0x0b26edf1, 0x66a01eab, 0x42224f9d, 0x42ce77ee, 0x664c26d8,
     0x0bcad582, 0x2f4884b4, 0xd0c73336, 0xf4456200, 0x99c3915a,
     0xbd41c06c, 0x981be1bc, 0xbc99b08a, 0xd11f43d0, 0xf59d12e6,
     0x0a12a564, 0x2e90f452, 0x43160708, 0x6794563e, 0x67786e4d,
     0x43fa3f7b, 0x2e7ccc21, 0x0afe9d17, 0xf5712a95, 0xd1f37ba3,
     0xbc7588f9, 0x98f7d9cf, 0x6019add5, 0x449bfce3, 0x291d0fb9,
     0x0d9f5e8f, 0xf210e90d, 0xd692b83b, 0xbb144b61, 0x9f961a57,
     0x9f7a2224, 0xbbf87312, 0xd67e8048, 0xf2fcd17e, 0x0d7366fc,
     0x29f137ca, 0x4477c490, 0x60f595a6, 0x45afb476, 0x612de540,
     0x0cab161a, 0x2829472c, 0xd7a6f0ae, 0xf324a198, 0x9ea252c2,
     0xba2003f4, 0xbacc3b87, 0x9e4e6ab1, 0xf3c899eb, 0xd74ac8dd,
     0x28c57f5f, 0x0c472e69, 0x61c1dd33, 0x45438c05, 0x2b759e93,
     0x0ff7cfa5, 0x62713cff, 0x46f36dc9, 0xb97cda4b, 0x9dfe8b7d,
     0xf0787827, 0xd4fa2911, 0xd4161162, 0xf0944054, 0x9d12b30e,
     0xb990e238, 0x461f55ba, 0x629d048c, 0x0f1bf7d6, 0x2b99a6e0,
     0x0ec38730, 0x2a41d606, 0x47c7255c, 0x6345746a, 0x9ccac3e8,
     0xb84892de, 0xd5ce6184, 0xf14c30b2, 0xf1a008c1, 0xd52259f7,
     0xb8a4aaad, 0x9c26fb9b, 0x63a94c19, 0x472b1d2f, 0x2aadee75,
     0x0e2fbf43},
    {0x00000000, 0x36f290f3, 0x6de521e6, 0x5b17b115, 0xdbca43cc,
     0xed38d33f, 0xb62f622a, 0x80ddf2d9, 0x6ce581d9, 0x5a17112a,
     0x0100a03f, 0x37f230cc, 0xb72fc215, 0x81dd52e6, 0xdacae3f3,
     0xec387300, 0xd9cb03b2, 0xef399341, 0xb42e2254, 0x82dcb2a7,
     0x0201407e, 0x34f3d08d, 0x6fe46198, 0x5916f16b, 0xb52e826b,
     0x83dc1298, 0xd8cba38d, 0xee39337e, 0x6ee4c1a7, 0x58165154,
     0x0301e041, 0x35f370b2, 0x68e70125, 0x5e1591d6, 0x050220c3,
     0x33f0b030, 0xb32d42e9, 0x85dfd21a, 0xdec8630f, 0xe83af3fc,
     0x040280fc, 0x32f0100f, 0x69e7a11a, 0x5f1531e9, 0xdfc8c330,
     0xe93a53c3, 0xb22de2d6, 0x84df7225, 0xb12c0297, 0x87de9264,
     0xdcc92371, 0xea3bb382, 0x6ae6415b, 0x5c14d1a8, 0x070360bd,
     0x31f1f04e, 0xddc9834e, 0xeb3b13bd, 0xb02ca2a8, 0x86de325b,
     0x0603c082, 0x30f15071, 0x6be6e164, 0x5d147197, 0xd1ce024a,
     0xe73c92b9, 0xbc2b23ac, 0x8ad9b35f, 0x0a044186, 0x3cf6d175,
     0x67e16060, 0x5113f093, 0xbd2b8393, 0x8bd91360, 0xd0cea275,
     0xe63c3286, 0x66e1c05f, 0x501350ac, 0x0b04e1b9, 0x3df6714a,
     0x080501f8, 0x3ef7910b, 0x65e0201e, 0x5312b0ed, 0xd3cf4234,
     0xe53dd2c7, 0xbe2a63d2, 0x88d8f321, 0x64e08021, 0x521210d2,
     0x0905a1c7, 0x3ff73134, 0xbf2ac3ed, 0x89d8531e, 0xd2cfe20b,
     0xe43d72f8, 0xb929036f, 0x8fdb939c, 0xd4cc2289, 0xe23eb27a,
     0x62e340a3, 0x5411d050, 0x0f066145, 0x39f4f1b6, 0xd5cc82b6,
     0xe33e1245, 0xb829a350, 0x8edb33a3, 0x0e06c17a, 0x38f45189,
     0x63e3e09c, 0x5511706f, 0x60e200dd, 0x5610902e, 0x0d07213b,
     0x3bf5b1c8, 0xbb284311, 0x8ddad3e2, 0xd6cd62f7, 0xe03ff204,
     0x0c078104, 0x3af511f7, 0x61e2a0e2, 0x57103011, 0xd7cdc2c8,
     0xe13f523b, 0xba28e32e, 0x8cda73dd, 0x78ed02d5, 0x4e1f9226,
     0x15082333, 0x23fab3c0, 0xa3274119, 0x95d5d1ea, 0xcec260ff,
     0xf830f00c, 0x1408830c, 0x22fa13ff, 0x79eda2ea, 0x4f1f3219,
     0xcfc2c0c0, 0xf9305033, 0xa227e126, 0x94d571d5, 0xa1260167,
     0x97d49194, 0xccc32081, 0xfa31b072, 0x7aec42ab, 0x4c1ed258,
     0x1709634d, 0x21fbf3be, 0xcdc380be, 0xfb31104d, 0xa026a158,
     0x96d431ab, 0x1609c372, 0x20fb5381, 0x7bece294, 0x4d1e7267,
     0x100a03f0, 0x26f89303, 0x7def2216, 0x4b1db2e5, 0xcbc0403c,
     0xfd32d0cf, 0xa62561da, 0x90d7f129, 0x7cef8229, 0x4a1d12da,
     0x110aa3cf, 0x27f8333c, 0xa725c1e5, 0x91d75116, 0xcac0e003,
     0xfc3270f0, 0xc9c10042, 0xff3390b1, 0xa42421a4, 0x92d6b157,
     0x120b438e, 0x24f9d37d, 0x7fee6268, 0x491cf29b, 0xa524819b,
     0x93d61168, 0xc8c1a07d, 0xfe33308e, 0x7eeec257, 0x481c52a4,
     0x130be3b1, 0x25f97342, 0xa923009f, 0x9fd1906c, 0xc4c62179,
     0xf234b18a, 0x72e94353, 0x441bd3a0, 0x1f0c62b5, 0x29fef246,
     0xc5c68146, 0xf33411b5, 0xa823a0a0, 0x9ed13053, 0x1e0cc28a,
     0x28fe5279, 0x73e9e36c, 0x451b739f, 0x70e8032d, 0x461a93de,
     0x1d0d22cb, 0x2bffb238, 0xab2240e1, 0x9dd0d012, 0xc6c76107,
     0xf035f1f4, 0x1c0d82f4, 0x2aff1207, 0x71e8a312, 0x471a33e1,
     0xc7c7c138, 0xf13551cb, 0xaa22e0de, 0x9cd0702d, 0xc1c401ba,
     0xf7369149, 0xac21205c, 0x9ad3b0af, 0x1a0e4276, 0x2cfcd285,
     0x77eb6390, 0x4119f363, 0xad218063, 0x9bd31090, 0xc0c4a185,
     0xf6363176, 0x76ebc3af, 0x4019535c, 0x1b0ee249, 0x2dfc72ba,
     0x180f0208, 0x2efd92fb, 0x75ea23ee, 0x4318b31d, 0xc3c541c4,
     0xf537d137, 0xae206022, 0x98d2f0d1, 0x74ea83d1, 0x42181322,
     0x190fa237, 0x2ffd32c4, 0xaf20c01d, 0x99d250ee, 0xc2c5e1fb,
     0xf4377108}};