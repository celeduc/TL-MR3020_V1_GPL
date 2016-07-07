#
# Makefile for USB Network drivers
#

obj-$(CONFIG_USB_CATC)		+= catc.o
obj-$(CONFIG_USB_KAWETH)	+= kaweth.o
obj-$(CONFIG_USB_PEGASUS)	+= pegasus.o
obj-$(CONFIG_USB_RTL8150)	+= rtl8150.o
obj-$(CONFIG_USB_HSO)		+= hso.o
#obj-$(CONFIG_USB_NET_AX8817X)	+= asix.o
obj-$(CONFIG_USB_NET_CDCETHER)	+= cdc_ether.o
obj-$(CONFIG_USB_NET_CDC_EEM)	+= cdc_eem.o
obj-$(CONFIG_USB_NET_DM9601)	+= dm9601.o
obj-$(CONFIG_USB_NET_SMSC95XX)	+= smsc95xx.o
obj-$(CONFIG_USB_NET_GL620A)	+= gl620a.o
obj-$(CONFIG_USB_NET_NET1080)	+= net1080.o
obj-$(CONFIG_USB_NET_PLUSB)	+= plusb.o
obj-$(CONFIG_USB_NET_RNDIS_HOST)	+= rndis_host.o
obj-$(CONFIG_USB_NET_CDC_SUBSET)	+= cdc_subset.o

obj-$(CONFIG_USB_NET_ZAURUS)	+= zaurus.o
obj-$(CONFIG_USB_NET_MCS7830)	+= mcs7830.o
obj-$(CONFIG_USB_USBNET)	+= usbnet.o
obj-$(CONFIG_USB_NET_INT51X1)	+= int51x1.o
obj-$(CONFIG_USB_CDC_PHONET)	+= cdc-phonet.o

CONFIG_USB_NET_CDC_NCM = m
obj-$(CONFIG_USB_NET_CDC_NCM)	+= cdc_ncm.o

CONFIG_USB_VL600 = m
obj-$(CONFIG_USB_VL600)		+= lg-vl600.o

#added by ZQQ,2012.03.16
#CONFIG_USB_NET_HUAWEI_WWAN = m
#obj-$(CONFIG_USB_NET_HUAWEI_WWAN)	+= huawei_wwan.o

CONFIG_USB_NET_CDC_ENC =m
obj-$(CONFIG_USB_NET_CDC_ENC)	+= cdc_enc.o
CONFIG_USB_QMI_WWAN = m

CONFIG_USB_QMI_WWAN =m
obj-$(CONFIG_USB_QMI_WWAN) += qmi_wwan.o
qmi_wwan-objs := qmi_proto.o qmi_wwan_core.o
#obj-$(CONFIG_USB_QMI_WWAN)	+= qmi_wwan_data.o
#qmi_wwan_data-objs := qmi_wwan.o

#CONFIG_USB_QMI_WWAN2 = m
#obj-$(CONFIG_USB_QMI_WWAN2)	+= qmi_wwan_contrl.o
#qmi_wwan_contrl-objs := qmi_wwan_core.o qmi_proto.o
#end add by ZQQ

CONFIG_USB_SIERRA_NET = m
obj-$(CONFIG_USB_SIERRA_NET)	+= sierra_net.o

CONFIG_USB_NET_RNDIS_HOST = m
obj-$(CONFIG_USB_SIERRA_NET)	+= rndis_host.o