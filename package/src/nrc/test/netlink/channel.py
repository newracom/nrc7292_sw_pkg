BW_1M = 1
BW_2M = 2
BW_4M = 4

CHANNEL_LIST = {
	"JP": {
		BW_1M: {
			9170: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5180},
			9180: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5185},
			9190: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5190},
			9200: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5195},
			9210: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5200},
			9220: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5205},
			9230: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5210},
			9240: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5215},
			9250: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5220},
			9260: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5225},
			9270: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5230}
		},
		BW_2M: {

		},
		BW_4M: {

		}
	},
	"KR": {
		BW_1M: {
			9255: {"upper_freq": 9260, "upper_bw": BW_2M, "nons1g": 0},
			9265: {"upper_freq": 9260, "upper_bw": BW_2M, "nons1g": 0},
			9275: {"upper_freq": 9280, "upper_bw": BW_2M, "nons1g": 0},
			9285: {"upper_freq": 9280, "upper_bw": BW_2M, "nons1g": 0},
			9295: {"upper_freq": 9300, "upper_bw": BW_2M, "nons1g": 0},
			9305: {"upper_freq": 9300, "upper_bw": BW_2M, "nons1g": 0},
			9315: {"upper_freq": 9320, "upper_bw": BW_2M, "nons1g": 0},
			9325: {"upper_freq": 9320, "upper_bw": BW_2M, "nons1g": 0},
			9335: {"upper_freq": 9340, "upper_bw": BW_2M, "nons1g": 0},
			9345: {"upper_freq": 9340, "upper_bw": BW_2M, "nons1g": 0},
			9355: {"upper_freq": 9360, "upper_bw": BW_2M, "nons1g": 0},
			9365: {"upper_freq": 9360, "upper_bw": BW_2M, "nons1g": 0}
		},
		BW_2M: {
			9260: {"upper_freq": 9270, "upper_bw": BW_4M, "nons1g": 0},
			9280: {"upper_freq": 9270, "upper_bw": BW_4M, "nons1g": 0},
			9300: {"upper_freq": 9310, "upper_bw": BW_4M, "nons1g": 0},
			9320: {"upper_freq": 9310, "upper_bw": BW_4M, "nons1g": 0},
			9340: {"upper_freq": 9350, "upper_bw": BW_4M, "nons1g": 0},
			9360: {"upper_freq": 9350, "upper_bw": BW_4M, "nons1g": 0}
		},
		BW_4M: {
			9270: {"upper_freq": 0, "upper_bw": 0, "nons1g": 0},
			9310: {"upper_freq": 0, "upper_bw": 0, "nons1g": 0},
			9350: {"upper_freq": 0, "upper_bw": 0, "nons1g": 0}
		}
		# BW_1M: {
		# 	9180: {"upper_freq": 5210, "upper_bw": BW_2M, "nons1g": 5180},
		# 	9190: {"upper_freq": 5210, "upper_bw": BW_2M, "nons1g": 5185},
		# 	9200: {"upper_freq": 9205, "upper_bw": BW_2M, "nons1g": 5190},
		# 	9210: {"upper_freq": 9205, "upper_bw": BW_2M, "nons1g": 5195},
		# 	9220: {"upper_freq": 9225, "upper_bw": BW_2M, "nons1g": 5200},
		# 	9230: {"upper_freq": 9225, "upper_bw": BW_2M, "nons1g": 5205},
		# 	9428: {"upper_freq": 9433, "upper_bw": BW_2M, "nons1g": 5230},
		# 	9438: {"upper_freq": 9433, "upper_bw": BW_2M, "nons1g": 5235},
		# 	9448: {"upper_freq": 9453, "upper_bw": BW_2M, "nons1g": 5240},
		# 	9458: {"upper_freq": 9453, "upper_bw": BW_2M, "nons1g": 5745}
		# },
		# BW_2M: {
		# 	9185: {"upper_freq": 0, "upper_bw": 0, "nons1g": 5210},
		# 	9205: {"upper_freq": 9215, "upper_bw": BW_4M, "nons1g": 5215},
		# 	9225: {"upper_freq": 9215, "upper_bw": BW_4M, "nons1g": 5220},
		# 	9433: {"upper_freq": 9443, "upper_bw": BW_4M, "nons1g": 5750},
		# 	9453: {"upper_freq": 9443, "upper_bw": BW_4M, "nons1g": 5755}
		# },
		# BW_4M: {
		# 	9215: {"upper_freq": 0, "upper_bw": 0, "nons1g": 5225},
		# 	9443: {"upper_freq": 0, "upper_bw": 0, "nons1g": 5760}
		# }
	},
	"TW": {
		BW_1M: {
			8390: {'upper_freq': 8395, 'upper_bw': BW_2M, "nons1g": 5180},
			8400: {'upper_freq': 8395, 'upper_bw': BW_2M, "nons1g": 5185},
			8410: {'upper_freq': 8415, 'upper_bw': BW_2M, "nons1g": 5190},
			8420: {'upper_freq': 8415, 'upper_bw': BW_2M, "nons1g": 5195},
			8430: {'upper_freq': 8435, 'upper_bw': BW_2M, "nons1g": 5200},
			8440: {'upper_freq': 8435, 'upper_bw': BW_2M, "nons1g": 5205},
			8450: {'upper_freq': 8455, 'upper_bw': BW_2M, "nons1g": 5210},
			8460: {'upper_freq': 8455, 'upper_bw': BW_2M, "nons1g": 5215},
			8470: {'upper_freq': 8475, 'upper_bw': BW_2M, "nons1g": 5220},
			8480: {'upper_freq': 8475, 'upper_bw': BW_2M, "nons1g": 5225},
			8490: {'upper_freq': 8495, 'upper_bw': BW_2M, "nons1g": 5230},
			8500: {'upper_freq': 8495, 'upper_bw': BW_2M, "nons1g": 5235},
			8510: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5240}
		},
		BW_2M: {
			8395: {'upper_freq': 8405, 'upper_bw': BW_4M, "nons1g": 5745},
			8415: {'upper_freq': 8405, 'upper_bw': BW_4M, "nons1g": 5750},
			8435: {'upper_freq': 8445, 'upper_bw': BW_4M, "nons1g": 5755},
			8455: {'upper_freq': 8445, 'upper_bw': BW_4M, "nons1g": 5760},
			8475: {'upper_freq': 8485, 'upper_bw': BW_4M, "nons1g": 5765},
			8495: {'upper_freq': 8485, 'upper_bw': BW_4M, "nons1g": 5770}
		},
		BW_4M: {
			8405: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5775},
			8445: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5780},
			8485: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5785}
		}
	},
	"US": {
		BW_1M: {
			9025: {'upper_freq': 0, 'upper_bw': BW_2M, "nons1g": 2412},
			9035: {'upper_freq': 0, 'upper_bw': BW_2M, "nons1g": 2422},
			9045: {'upper_freq': 9050, 'upper_bw': BW_2M, "nons1g": 2432},
			9055: {'upper_freq': 9050, 'upper_bw': BW_2M, "nons1g": 2442},
			9065: {'upper_freq': 9070, 'upper_bw': BW_2M, "nons1g": 2452},
			9075: {'upper_freq': 9070, 'upper_bw': BW_2M, "nons1g": 2462},
			9085: {'upper_freq': 9090, 'upper_bw': BW_2M, "nons1g": 5180},
			9095: {'upper_freq': 9090, 'upper_bw': BW_2M, "nons1g": 5185},
			9105: {'upper_freq': 9110, 'upper_bw': BW_2M, "nons1g": 5190},
			9115: {'upper_freq': 9110, 'upper_bw': BW_2M, "nons1g": 5195},
			9125: {'upper_freq': 9130, 'upper_bw': BW_2M, "nons1g": 5200},
			9135: {'upper_freq': 9130, 'upper_bw': BW_2M, "nons1g": 5205},
			9145: {'upper_freq': 9150, 'upper_bw': BW_2M, "nons1g": 5210},
			9155: {'upper_freq': 9150, 'upper_bw': BW_2M, "nons1g": 5215},
			9165: {'upper_freq': 9170, 'upper_bw': BW_2M, "nons1g": 5220},
			9175: {'upper_freq': 9170, 'upper_bw': BW_2M, "nons1g": 5225},
			9185: {'upper_freq': 9190, 'upper_bw': BW_2M, "nons1g": 5230},
			9195: {'upper_freq': 9190, 'upper_bw': BW_2M, "nons1g": 5235},
			9205: {'upper_freq': 9210, 'upper_bw': BW_2M, "nons1g": 5240},
			9215: {'upper_freq': 9210, 'upper_bw': BW_2M, "nons1g": 5500},
			9225: {'upper_freq': 9230, 'upper_bw': BW_2M, "nons1g": 5520},
			9235: {'upper_freq': 9230, 'upper_bw': BW_2M, "nons1g": 5540},
			9245: {'upper_freq': 9250, 'upper_bw': BW_2M, "nons1g": 5745},
			9255: {'upper_freq': 9250, 'upper_bw': BW_2M, "nons1g": 5750},
			9265: {'upper_freq': 9270, 'upper_bw': BW_2M, "nons1g": 5755},
			9275: {'upper_freq': 9270, 'upper_bw': BW_2M, "nons1g": 5760}
		},
		BW_2M: {
			9030: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 2417},
			9050: {'upper_freq': 9060, 'upper_bw': BW_4M, "nons1g": 2437},
			9070: {'upper_freq': 9060, 'upper_bw': BW_4M, "nons1g": 2457},
			9090: {'upper_freq': 9100, 'upper_bw': BW_4M, "nons1g": 5560},
			9110: {'upper_freq': 9100, 'upper_bw': BW_4M, "nons1g": 5765},
			9130: {'upper_freq': 9140, 'upper_bw': BW_4M, "nons1g": 5770},
			9150: {'upper_freq': 9140, 'upper_bw': BW_4M, "nons1g": 5775},
			9170: {'upper_freq': 9180, 'upper_bw': BW_4M, "nons1g": 5780},
			9190: {'upper_freq': 9180, 'upper_bw': BW_4M, "nons1g": 5785},
			9210: {'upper_freq': 9220, 'upper_bw': BW_4M, "nons1g": 5790},
			9230: {'upper_freq': 9220, 'upper_bw': BW_4M, "nons1g": 5795},
			9250: {'upper_freq': 9260, 'upper_bw': BW_4M, "nons1g": 5800},
			9270: {'upper_freq': 9260, 'upper_bw': BW_4M, "nons1g": 5805}
		},
		BW_4M: {
			9060: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 2447},
			9100: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5580},
			9140: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5810},
			9180: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5815},
			9220: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5820},
			9260: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5825}
		},
	},
	"EU": {
		BW_1M: {
			8635: {'upper_freq': 8640, 'upper_bw': BW_2M, "nons1g": 5180},
			8645: {'upper_freq': 8640, 'upper_bw': BW_2M, "nons1g": 5185},
			8655: {'upper_freq': 8660, 'upper_bw': BW_2M, "nons1g": 5190},
			8665: {'upper_freq': 8660, 'upper_bw': BW_2M, "nons1g": 5195},
			8675: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5200}
		},
		BW_2M: {
			8640: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5205},
			8660: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5210}
		},
		BW_4M: {

		}
	},
	"CN": {
		BW_1M: {
			7555: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5180},
			7565: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5185},
			7575: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5190},
			7585: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5195},
			7595: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5200},
			7605: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5205},
			7615: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5210},
			7625: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5215},
			7635: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5220},
			7645: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5225},
			7655: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5230},
			7665: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5235},
			7675: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5240},
			7685: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5745},
			7695: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5750},
			7705: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5755},
			7795: {'upper_freq': 7800, 'upper_bw': BW_2M, "nons1g": 5760},
			7805: {'upper_freq': 7800, 'upper_bw': BW_2M, "nons1g": 5765},
			7815: {'upper_freq': 7820, 'upper_bw': BW_2M, "nons1g": 5770},
			7825: {'upper_freq': 7820, 'upper_bw': BW_2M, "nons1g": 5775},
			7835: {'upper_freq': 7840, 'upper_bw': BW_2M, "nons1g": 5780},
			7845: {'upper_freq': 7840, 'upper_bw': BW_2M, "nons1g": 5785},
			7855: {'upper_freq': 7860, 'upper_bw': BW_2M, "nons1g": 5790},
			7865: {'upper_freq': 7860, 'upper_bw': BW_2M, "nons1g": 5795}
		},
		BW_2M: {
			7800: {'upper_freq': 7810, 'upper_bw': BW_4M, "nons1g": 5800},
			7820: {'upper_freq': 7810, 'upper_bw': BW_4M, "nons1g": 5805},
			7840: {'upper_freq': 7850, 'upper_bw': BW_4M, "nons1g": 5810},
			7860: {'upper_freq': 7850, 'upper_bw': BW_4M, "nons1g": 5815}
		},
		BW_4M: {
			7810: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5820},
			7850: {'upper_freq': 0, 'upper_bw': 0, "nons1g": 5825}
		}
	}
}