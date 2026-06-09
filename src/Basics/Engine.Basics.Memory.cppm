module;

#include <cstdint>
#include <cstdio>

export module Engine.Basics.Memory;

export namespace Engine {
    namespace Basics {
        namespace Memory {
            void dump_hex(const uint8_t *buf, uint32_t size)
            {
                int i, j;

                for (i = 0; i < size; i += 16)
                {
                    printf("%08X: ", i);

                    for (j = 0; j < 16; j++)
                    {
                        if (i + j < size)
                        {
                            printf("%02X ", buf[i + j]);
                        }
                        else
                        {
                            printf("   ");
                        }
                    }
                    printf(" ");

                    for (j = 0; j < 16; j++)
                    {
                        if (i + j < size)
                        {
                            printf("%c", ((unsigned int)((buf[i + j]) - ' ') < 127u - ' ')
                            ? buf[i + j]
                            : '.');
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}
