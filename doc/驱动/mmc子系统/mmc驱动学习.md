# MMC驱动学习


sdhci_esdhc_imx_pdata->ops
static struct sdhci_ops sdhci_esdhc_ops = {
	.read_l = esdhc_readl_le,
	.read_w = esdhc_readw_le,
	.write_l = esdhc_writel_le,
	.write_w = esdhc_writew_le,
	.write_b = esdhc_writeb_le,
	.set_clock = esdhc_pltfm_set_clock,
	.get_max_clock = esdhc_pltfm_get_max_clock,
	.get_min_clock = esdhc_pltfm_get_min_clock,
	.get_max_timeout_count = esdhc_get_max_timeout_count,
	.get_ro = esdhc_pltfm_get_ro,
	.set_timeout = esdhc_set_timeout,
	.set_bus_width = esdhc_pltfm_set_bus_width,
	.set_uhs_signaling = esdhc_set_uhs_signaling,
	.reset = esdhc_reset,
	.hw_reset = esdhc_hw_reset,
};

**sdhci_pltfm_init**

->sdhci_alloc_host->mmc_alloc_host->mmc_rescan->mmc_rescan_try_freq->mmc_attach_mmc->mmc_attach_bus->mmc_ops->mmc_





## mmc bus.c

/*

Allocate and initialise a new MMC card structure.
*/
struct mmc_card *mmc_alloc_card(struct mmc_host *host, struct device_type *type)
{
struct mmc_card *card;

card = kzalloc(sizeof(struct mmc_card), GFP_KERNEL);
if (!card)
	return ERR_PTR(-ENOMEM);

card->host = host; // 完成host与card的绑定

device_initialize(&card->dev);

card->dev.parent = mmc_classdev(host);
card->dev.bus = &mmc_bus_type;        //将card绑定在mmc总线上
card->dev.release = mmc_release_card;
card->dev.type = type;

return card;
}

mmc_add_card()

将card添加到bus的device上：例如mmc0 mmc1

## mmc host.c

host的驱动

/**
 *	mmc_alloc_host - initialise the per-host structure.
 *	@extra: sizeof private data structure
 *	@dev: pointer to host device model structure
 *
 *	Initialise the per-host structure.
 */
    struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
    {
	int err;
	struct mmc_host *host;
	int alias_id, min_idx, max_idx;

	host = kzalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
	if (!host)
		return NULL;

	/* scanning will be enabled when we're ready */
	host->rescan_disable = 1;
	idr_preload(GFP_KERNEL);
	spin_lock(&mmc_host_lock);

	host->parent = dev;//设置从属关系
	alias_id = mmc_get_reserved_index(host);
	if (alias_id >= 0) {
		min_idx = alias_id;
		max_idx = alias_id + 1;
	} else {
		min_idx = mmc_first_nonreserved_index();
		max_idx = 0;
	}

	err = idr_alloc(&mmc_host_idr, host, min_idx, max_idx, GFP_NOWAIT);
	if (err >= 0)
		host->index = err;
	spin_unlock(&mmc_host_lock);
	idr_preload_end();
	if (err < 0) {
		kfree(host);
		return NULL;
	}

	dev_set_name(&host->class_dev, "mmc%d", host->index);

	host->class_dev.parent = dev;
	host->class_dev.class = &mmc_host_class;
	device_initialize(&host->class_dev);

	if (mmc_gpio_alloc(host)) {
		put_device(&host->class_dev);
		return NULL;
	}

	mmc_host_clk_init(host);

	spin_lock_init(&host->lock);
	init_waitqueue_head(&host->wq);
	INIT_DELAYED_WORK(&host->detect, mmc_rescan);
    #ifdef CONFIG_PM
	host->pm_notify.notifier_call = mmc_pm_notify;
    #endif
	setup_timer(&host->retune_timer, mmc_retune_timer, (unsigned long)host);

	/*
	 * By default, hosts do not support SGIO or large requests.
	 * They have to set these according to their abilities.
	 */
	    host->max_segs = 1;
	    host->max_seg_size = PAGE_CACHE_SIZE;

	host->max_req_size = PAGE_CACHE_SIZE;
	host->max_blk_size = 512;
	host->max_blk_count = PAGE_CACHE_SIZE / 512;

	return host;
    }

 **mmc host申请调用流程**

static struct platform_driver sdhci_esdhc_imx_driver = {
	.driver		= {
		.name	= "sdhci-esdhc-imx",
		.of_match_table = imx_esdhc_dt_ids,
		.pm	= &sdhci_esdhc_pmops,
	},
	.id_table	= imx_esdhc_devtype,
	.probe		= sdhci_esdhc_imx_probe,
	.remove		= sdhci_esdhc_imx_remove,
};

sdhci_esdhc_imx_probe->sdhci_pltfm_init->sdhci_alloc_host->mmc_alloc_host



## mmc mmc.c

```C
static const struct mmc_bus_ops mmc_ops = {
	.remove = mmc_remove,
	.detect = mmc_detect,
	.suspend = mmc_suspend,
	.resume = mmc_resume,
	.runtime_suspend = mmc_runtime_suspend,
	.runtime_resume = mmc_runtime_resume,
	.alive = mmc_alive,
	.shutdown = mmc_shutdown,
	.reset = mmc_reset,
};
```



mmc_attach_mmc(struct mmc_host *host)

- 设置总线模式

- 选择一个card和host都支持的最低工作电压

- 对于不同type的card，相应mmc总线上的操作协议也可能有所不同。所以需要设置相应的总线

  作集合（mmc_host->bus_ops

- 初始化card使其进入工作状态（mmc_init_card）

- 为card构造对应的mmc_card并且注册到mmc bus中

  

  ```c
  /*
   * Starting point for MMC card init.
   */
  int mmc_attach_mmc(struct mmc_host *host)
  {
  	int err;
  	u32 ocr, rocr;
  
  	BUG_ON(!host);
  	WARN_ON(!host->claimed);
  
  	/* Set correct bus mode for MMC before attempting attach */
  	//确定正确的总线模式
  	if (!mmc_host_is_spi(host))
  		mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);
  
  	// 发送CMD1命令（MMC_SEND_OP_COND），并且参数为0，获取OCR
  	err = mmc_send_op_cond(host, 0, &ocr);
  	if (err)
  		return err;
  //设置host的ops
  	mmc_attach_bus(host, &mmc_ops);
  	if (host->ocr_avail_mmc)
  		host->ocr_avail = host->ocr_avail_mmc;
  
  	/*
  	 * We need to get OCR a different way for SPI.
  	 */
  	if (mmc_host_is_spi(host)) {
  		err = mmc_spi_read_ocr(host, 1, &ocr);
  		if (err)
  			goto err;
  	}
  
  	rocr = mmc_select_voltage(host, ocr);// 通过OCR寄存器选择一个HOST和card都支持的最低电压
  
  	/*
  	 * Can we support the voltage of the card?
  	 */
  	if (!rocr) {
  		err = -EINVAL;
  		goto err;
  	}
  
  	/*
  	 * Detect and init the card.
  	 */
  	/* 调用mmc_init_card初始化该mmc type card，这里是核心函数*/
  	err = mmc_init_card(host, rocr, NULL);
  	if (err)
  		goto err;
  
  	mmc_release_host(host);
  // 调用到mmc_add_card，该mmc_card就挂在了mmc_bus上，会和mmc_bus上的block这类mmc driver匹配起来
  	err = mmc_add_card(host->card);
  	mmc_claim_host(host);
  	if (err)
  		goto remove_card;
  
  	return 0;
  
  remove_card:
  	mmc_release_host(host);
  	mmc_remove_card(host->card);
  	mmc_claim_host(host);
  	host->card = NULL;
  err:
  	mmc_detach_bus(host);
  
  	pr_err("%s: error %d whilst initialising MMC card\n",
  		mmc_hostname(host), err);
  
  	return err;
  }
  
  ```
  
  
  重点函数
  
  
  
  
  
  ```c
  static int mmc_init_card(struct mmc_host *host, u32 ocr,
      struct mmc_card *oldcard)
  {
  // struct mmc_host *host：该mmc card使用的host
  // ocr：表示了host要使用的电压，在mmc_attach_mmc中，已经得到了一个HOST和card都支持的最低电压  struct mmc_card *card;
      int err = 0;
      u32 cid[4];
      u32 rocr;
      u8 *ext_csd = NULL;
  
      BUG_ON(!host);
      WARN_ON(!host->claimed);
  
      /* Set correct bus mode for MMC before attempting init */
      if (!mmc_host_is_spi(host))
          mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);  // 设置总线模式为开漏模式
  
  /* 根据mmc协议从mmc总线上选中一张card（协议的初始化流程） */
      mmc_go_idle(host);
          // 发送CMD0指令，GO_IDLE_STATE
          // 使mmc card进入idle state。
          // 虽然进入到了Idle State，但是上电复位过程并不一定完成了，这主要靠读取OCR的busy位来判断，而流程归结为下一步。
  
      /* The extra bit indicates that we support high capacity */
      err = mmc_send_op_cond(host, ocr | (1 << 30), &rocr);
          // 发送CMD1指令，SEND_OP_COND
          // 这里会设置card的工作电压寄存器OCR，并且通过busy位（bit31）来判断card的上电复位过程是否完成，如果没有完成的话需要重复发送。
          // 完成之后，mmc card进入ready state。
  
      /*
       * Fetch CID from card.
       */
      if (mmc_host_is_spi(host))
          err = mmc_send_cid(host, cid);
      else
          err = mmc_all_send_cid(host, cid);
                  // 这里会发送CMD2指令，ALL_SEND_CID
                  // 广播指令，使card回复对应的CID寄存器的值。在这里就相应获得了CID寄存器的值了，存储在cid中。
                  // 完成之后，MMC card会进入Identification State。
  
      if (oldcard) {
  。。。
      } else {
  /* 调用mmc_alloc_card分配一个mmc_card并进行部分设置 */
          card = mmc_alloc_card(host, &mmc_type); 
                  // 为card配分一个struct mmc_card结构体并进行初始化，在mmc_type中为mmc定义了大量的属性。
                  // 具体参考“《mmc core——bus模块说明》——》mmc_alloc_card”
          card->type = MMC_TYPE_MMC; // 设置card的type为MMC_TYPE_MMC
          card->rca = 1;  // 设置card的RCA地址为1
          memcpy(card->raw_cid, cid, sizeof(card->raw_cid));      // 将读到的CID存储到card->raw_cid，也就是原始CID值中
          card->reboot_notify.notifier_call = mmc_reboot_notify;
          host->card = card;      // 将mmc_card和mmc_host 进行关联
      }
  
  
  /* 设置card RCA地址 */
      if (!mmc_host_is_spi(host)) {
          err = mmc_set_relative_addr(card);
                  // 发送CMD3指令，SET_RELATIVE_ADDR
                  // 设置该mmc card的关联地址为card->rca，也就是0x0001
                  // 完成之后，该MMC card进入standby模式。
  
          mmc_set_bus_mode(host, MMC_BUSMODE_PUSHPULL);
                  // 设置总线模式为MMC_BUSMODE_PUSHPULL
      }
  
  /* 从card的csd寄存器以及ext_csd寄存器获取信息并设置到mmc_card的相应成员中 */
      if (!oldcard) {
          /*
           * Fetch CSD from card.
           */
          err = mmc_send_csd(card, card->raw_csd);
                  // 发送CMD9指令，MMC_SEND_CSD
                  // 要求mmc card发送csd寄存器，存储到card->raw_csd中，也就是原始的csd寄存器的值。
                  // 此时mmc card还是处于standby state
  
          err = mmc_decode_csd(card);
                  // 解析raw_csd，获取到各个bit的值并设置到card->csd中的相应成员上
  
          err = mmc_decode_cid(card);
                  // 解析raw_cid，获取到各个bit的值并设置到card->cid中的相应成员上
      }
  
      /*
       * Select card, as all following commands rely on that.
       */
      if (!mmc_host_is_spi(host)) {
          err = mmc_select_card(card);
                  // 发送CMD7指令，SELECT/DESELECT CARD
                  // 选择或者断开指定的card
                  // 这时卡进入transfer state。后续可以通过各种指令进入到receive-data state或者sending-data state依次来进行数据的传输
      }
  
      if (!oldcard) {
          err = mmc_get_ext_csd(card, &ext_csd);
                  // 发送CMD8指令，SEND_EXT_CSD
                  // 这里要求处于transfer state的card发送ext_csd寄存器，这里获取之后存放在ext_csd寄存器中
                  // 这里会使card进入sending-data state，完成之后又退出到transfer state。
  
          card->cached_ext_csd = ext_csd;    // 将ext_csd原始值存储到card->cached_ext_csd，表示用来保存ext_csd的一块缓存，可能还没有和card的ext_csd同步
          err = mmc_read_ext_csd(card, ext_csd);  // 解析ext_csd的值，获取到各个bit的值并设置到card->ext_csd中的相应成员上
  
          if (!(mmc_card_blockaddr(card)) && (rocr & (1<<30)))
              mmc_card_set_blockaddr(card);
  
          /* Erase size depends on CSD and Extended CSD */
          mmc_set_erase_size(card);  // 设置card的erase_size，扇区里面的擦除字节数，读出来是512K
  
          if (card->ext_csd.sectors && (rocr & MMC_CARD_SECTOR_ADDR))
              mmc_card_set_blockaddr(card);
      }
  
  /* 根据host属性以及一些需求修改ext_csd寄存器的值 */
      /*
       * If enhanced_area_en is TRUE, host needs to enable ERASE_GRP_DEF
       * bit.  This bit will be lost every time after a reset or power off.
       */
      if (card->ext_csd.enhanced_area_en ||
          (card->ext_csd.rev >= 3 && (host->caps2 & MMC_CAP2_HC_ERASE_SZ))) {
          err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
                   EXT_CSD_ERASE_GROUP_DEF, 1,
                   card->ext_csd.generic_cmd6_time);
                  // 发送CMD6命令，MMC_SWITCH
                  // 用于设置ext_csd寄存器的某些bit
                  // 当enhanced_area_en 被设置的时候，host需要去设置ext_csd寄存器中的EXT_CSD_ERASE_GROUP_DEF位为1
      }
  
      if (card->ext_csd.part_config & EXT_CSD_PART_CONFIG_ACC_MASK) {
          card->ext_csd.part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
          err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONFIG,
                   card->ext_csd.part_config,
                   card->ext_csd.part_time);
                  // 发送CMD6命令，MMC_SWITCH
                  // 用于设置ext_csd寄存器的某些bit
                  // 设置ext_csd寄存器中的EXT_CSD_CMD_SET_NORMAL位为EXT_CSD_PART_CONFIG
          card->part_curr = card->ext_csd.part_config &
                    EXT_CSD_PART_CONFIG_ACC_MASK;
      }
  
      if ((host->caps2 & MMC_CAP2_POWEROFF_NOTIFY) &&
          (card->ext_csd.rev >= 6)) {
          err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
                   EXT_CSD_POWER_OFF_NOTIFICATION,
                   EXT_CSD_POWER_ON,
                   card->ext_csd.generic_cmd6_time);
                   // 发送CMD6命令，MMC_SWITCH
                  // 用于设置ext_csd寄存器的某些bit
                  // 设置ext_csd寄存器中的EXT_CSD_POWER_OFF_NOTIFICATION位为EXT_CSD_POWER_ON
      }
  
  /* 设置mmc总线时钟频率以及位宽 */
      err = mmc_select_bus_speed(card, ext_csd); // 激活host和card都支持的最大总线速度
          //.........这里过滤掉一些设置ext_csd的代码
      if (!oldcard) {
  
          if (card->ext_csd.bkops_en) {
              INIT_DELAYED_WORK(&card->bkops_info.dw,
                        mmc_start_idle_time_bkops);
                          // 如果emmc支持bkops的话，就初始化card->bkops_info.dw工作为mmc_start_idle_time_bkops
          }
      }
  
      return 0;
  }
  ```
  
  
  
  - 根据协议初始化mmc type card，使其进入相应状态（standby state）
  - 为mmc type card构造对应mmc_card并进行设置
  - 从card的csd寄存器以及ext_csd寄存器获取card信息并设置到mmc_card的相应成员中
  - 根据host属性以及一些需求修改ext_csd寄存器的值
  - 设置mmc总线时钟频率以及位宽
    

​	mmc检测

mmc_alloc_host->mmc_rescan->mmc_rescan_try_freq->mmc_attach_mmc

sdhci_add_host->sdhci_thread_irq->mmc_detect_change->_mmc_detect_change->mmc_schedule_delayed_work(&host->detect, delay);

mmc 检测流程图

<img src="C:\Users\he\Desktop\linux-study-word\mmc驱动子系统\image-20240324165316596.png" alt="image-20240324165316596" style="zoom: 67%;" />





![人脸布控流程图 (1)](F:\download\人脸布控流程图 (1).png)



# MMC读写流程

mmc_blk_probe->mmc_blk_alloc->mmc_blk_alloc_req->mmc_blk_issue_rq->mmc_blk_issue_rw_rq->mmc_start_req-> mmc_start_request ->__mmc_start_request->*host*->ops->request(*host*, *mrq*)-> sdhci_request ->sdhci_send_command->sdhci_writew                 
