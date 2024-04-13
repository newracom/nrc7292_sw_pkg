# SAE Hash-to-Element (H2E) Support

## Background

The Dragonblood paper(https://wpa3.mathyvanhoef.com/), which was published in 2019, revealed the vulnerability of SAE, the core technology of WPA3. In response to this vulnerability, IEEE 802.11 introduced a new method called "Hash-to-Element" (H2E) to enhance SAE. The H2E method is utilized in the derivation of the secret PWE (Password Element) during SAE authentication.

Subsequently, the Wi-Fi Alliance (WFA) updated its specifications and test plans to make the SAE Hash-to-Element (H2E) method a mandatory feature. Please refer to slides 7 and 22 of [Wi-Fi Alliance® Wi-Fi® Security Roadmap and 
WPA3™Updates](https://www.wi-fi.org/system/files/202012_Wi-Fi_Security_Roadmap_and_WPA3_Updates.pdf). For HaLow certification, three security certifications are required as prerequisites: SAE, OWE, and PMF. SAE certification is one of the necessary components.

Support for SAE H2E was implemented in the Linux mac80211 layer on November 6, 2020.

You can find more information about this implementation in here: [cfg80211: Add support to configure SAE PWE value to drivers](https://git.kernel.org/pub/scm/linux/kernel/git/jberg/mac80211.git/commit/?id=9f0ffa418483938d25a15f6ad3891389f333bc59). Various related patches have also been applied for the latest security enhancements.

## Policy

Newracom has been supporting H2E since HaLow certification program launch.
H2E has been INTENTIONALLY disabled due to the following reason:
- To support H2E on STA, the kernel version should be higher than 5.10.x and wpa_supplicant version should be at least 2.10. However, many of Newracom customers are using old versions of kernel. So, Newracom INTENTIONALLY turned that off in the released SW packages.

## Current Status

### Host mode: Support H2E but disabled

Default setting of hostapd/wpa_supplicant.conf files
```
# SAE mechanism for PWE derivation
# 0 = hunting-and-pecking loop only (default without password identifier)
# 1 = hash-to-element only (default with password identifier)
# 2 = both hunting-and-pecking loop and hash-to-element enabled
# Note: The default value is likely to change from 0 to 2 once the new
# hash-to-element mechanism has received more interoperability testing.
# When using SAE password identifier, the hash-to-element mechanism is used
# regardless of the sae_pwe parameter value.
#sae_pwe=0
```

## H2E Activation

For the HaLow certification or interoperability with other HaLow certified devices, sae_pwe=1 should be added in the  hostapd/wpa_supplicant configuration files.

In addition, the kernel should be upgraded to 5.10.x for the STA EVK.
