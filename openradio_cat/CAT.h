#define CAT_LOCK_ON			0x00
#define CAT_LOCK_OFF			0x80
#define CAT_PTT_ON			0x08
#define CAT_PTT_OFF			0x88
#define CAT_FREQ_SET			0x01
#define CAT_MODE_SET			0x07
#define CAT_MODE_LSB			0x00
#define CAT_MODE_USB			0x01
#define CAT_MODE_CW			0x02
#define CAT_MODE_CWR			0x03
#define CAT_MODE_AM			0x04
#define CAT_MODE_FM			0x08
#define CAT_MODE_DIG			0x0A
#define CAT_MODE_PKT			0x0C
#define CAT_MODE_FMN			0x88
#define CAT_CLAR_ON			0x05
#define CAT_CLAR_OFF			0x85
#define CAT_CLAR_SET			0xF5
#define CAT_VFO_AB			0x81
#define CAT_SPLIT_ON			0x02
#define CAT_SPLIT_OFF			0x82
#define CAT_RPTR_OFFSET_CMD		0x09
#define CAT_RPTR_OFFSET_N		0x09 // -
#define CAT_RPTR_OFFSET_P		0x49 // +
#define CAT_RPTR_OFFSET_S		0x89 // simlex
#define CAT_RPTR_FREQ_SET		0xF9
#define CAT_SQL_CMD			0x0A //{P1 ,0x00,0x00,0x00,0x0A}
#define CAT_SQL_DCS			0x0A // all values below are P1
#define CAT_SQL_DCS_DECD		0x0B // only useful in "split"
#define CAT_SQL_DCS_ENCD		0x0C
#define CAT_SQL_CTCSS			0x2A
#define CAT_SQL_CTCSS_DECD		0x3A
#define CAT_SQL_CTCSS_ENCD		0x4A
#define CAT_SQL_OFF			0x8A
#define CAT_SQL_CTCSS_SET		0x0B
#define CAT_SQL_DCS_SET		0x0C
#define CAT_RX_DATA_CMD		0xE7
#define CAT_TX_DATA_CMD		0xF7
#define CAT_RX_FREQ_CMD		0x03
#define CAT_NULL_DATA			0x00
