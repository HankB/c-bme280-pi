#include "bme280.h"

#include <fcntl.h>
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "compensation.h"

//static const int buf_len = 128;
#define buf_len 128
static char buf[buf_len];
#define MAX_TRIES 5

int main(void) {
    int fd = 0;
    uint8_t dataBlock[8];
    int32_t temp_int = 0;
    int32_t press_int = 0;
    int32_t hum_int = 0;
    //double station_press = 0.0;

    /* open i2c comms */
    if ((fd = open(DEV_PATH, O_RDWR)) < 0) {
        perror("Unable to open i2c device");
        return ENXIO;
    }

    /* configure i2c slave */
    if (ioctl(fd, I2C_SLAVE, DEV_ID) < 0) {
        perror("Unable to configure i2c slave device");
        close(fd);
        return EBADE;
    }

    /* check our identification */
    if (i2c_smbus_read_byte_data(fd, IDENT) != 0x60) {
        perror("device ident error");
        close(fd);
        return ENODEV;
    }

    /* device soft reset */
    i2c_smbus_write_byte_data(fd, SOFT_RESET, 0xB6);
    usleep(50000);

    /* read and set compensation parameters */
    setCompensationParams(fd);

    /* humidity o/s x 1 */
    i2c_smbus_write_byte_data(fd, CTRL_HUM, 0x1);

    /* filter off */
    i2c_smbus_write_byte_data(fd, CONFIG, 0);

    /* set forced mode, pres o/s x 1, temp o/s x 1 and take 1st reading */
    i2c_smbus_write_byte_data(fd, CTRL_MEAS, 0x25);

    for (int i=0;i<MAX_TRIES;i++) {
        /* Sleep for 1 second for demonstration purposes.
         * Data can be streamed with a sleep time down
         * to 10 ms [usleep(10000)] with oversampling set at x1.
         * See section 9, appendix B of the Bosch technical
         * datasheet for details on measurement time calculation.
         */
	usleep(10000);

        /* check data is ready to read */
        if ((i2c_smbus_read_byte_data(fd, STATUS) & 0x9) != 0) {
            printf("%s\n", "Error, data not ready");
        }
	else
	{
	    /* read data registers */
	    i2c_smbus_read_i2c_block_data(fd, DATA_START_ADDR, DATA_LENGTH, dataBlock);

	    /* awake and take next reading */
	    i2c_smbus_write_byte_data(fd, CTRL_MEAS, 0x25);

	    /* get raw temp */
	    temp_int = (dataBlock[3] << 16 | dataBlock[4] << 8 | dataBlock[5]) >> 4;

	    /* get raw pressure */
	    press_int = (dataBlock[0] << 16 | dataBlock[1] << 8 | dataBlock[2]) >> 4;

	    /* get raw humidity */
	    hum_int = dataBlock[6] << 8 | dataBlock[7];

	    /* calculate and print compensated temp. This function is called first, as it also sets the
	     * t_fine global variable required by the next two function calls
	     */

	    // format JSON content
	    snprintf(buf, buf_len, "{ \"t\":%lld, \"temp\":%.2f, \"press\":%.2f, \"humid\":%.2f, \"device\":\"BME280\"}", 
		    time(0),
		    BME280_compensate_T_double(temp_int)/5.0*9.0+32.0,
		    BME280_compensate_P_double(press_int) / 100.0,
		    BME280_compensate_H_double(hum_int));
	    printf("%s\n", buf);
	    return 0;
	}
    }

    return ETIMEDOUT;
}

double sta2sea(double station_press) {
    return station_press * exp((-M * G * -LOCAL_HASL) / (R * T));
}
