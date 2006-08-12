/*
 * Copyright (c) 2004, 2005
 *      Damien Bergamini <damien.bergamini@free.fr>.
 * Copyright (c) 2004, 2005
 *     Andrew Atrens <atrens@nortelnetworks.com>.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/iwi/if_iwivar.h,v 1.4.2.1 2005/09/26 17:31:36 damien Exp $
 * $DragonFly: src/sys/dev/netif/iwi/if_iwivar.h,v 1.5.2.1 2006/08/12 14:04:17 sephe Exp $
 */

struct iwi_firmware {
	void	*boot;
	int	boot_size;
	void	*ucode;
	int	ucode_size;
	void	*main;
	int	main_size;
};

struct iwi_rx_radiotap_header {
	struct ieee80211_radiotap_header wr_ihdr;
	uint8_t		wr_flags;
	uint8_t		wr_rate;
	uint16_t	wr_chan_freq;
	uint16_t	wr_chan_flags;
	uint8_t		wr_antsignal;
	uint8_t		wr_antenna;
};

#define IWI_RX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_RATE) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |				\
	 (1 << IEEE80211_RADIOTAP_DB_ANTSIGNAL) |			\
	 (1 << IEEE80211_RADIOTAP_ANTENNA))

struct iwi_tx_radiotap_header {
	struct ieee80211_radiotap_header wt_ihdr;
	uint8_t		wt_flags;
	uint16_t	wt_chan_freq;
	uint16_t	wt_chan_flags;
};

#define IWI_TX_RADIOTAP_PRESENT						\
	((1 << IEEE80211_RADIOTAP_FLAGS) |				\
	 (1 << IEEE80211_RADIOTAP_CHANNEL))

struct iwi_cmd_ring {
	bus_dma_tag_t		desc_dmat;
	bus_dmamap_t		desc_map;
	bus_addr_t		physaddr;
	struct iwi_cmd_desc	*desc;
	int			count;
	int			queued;
	int			cur;
	int			next;
};

struct iwi_tx_data {
	bus_dmamap_t		map;
	struct mbuf		*m;
	struct ieee80211_node	*ni;
};

struct iwi_tx_ring {
	bus_dma_tag_t		desc_dmat;
	bus_dma_tag_t		data_dmat;
	bus_dmamap_t		desc_map;
	bus_addr_t		physaddr;
	bus_addr_t		csr_ridx;
	bus_addr_t		csr_widx;
	struct iwi_tx_desc	*desc;
	struct iwi_tx_data	*data;
	int			count;
	int			queued;
	int			cur;
	int			next;
};

struct iwi_rx_data {
	bus_dmamap_t	map;
	bus_addr_t	physaddr;
	uint32_t	reg;
	struct mbuf	*m;
};

struct iwi_rx_ring {
	bus_dma_tag_t		data_dmat;
	struct iwi_rx_data	*data;
	int			count;
	int			cur;
};

struct iwi_node {
	struct ieee80211_node	in_node;
	int			in_station;
#define IWI_MAX_IBSSNODE_NBYTE	4
#define IWI_MAX_IBSSNODE	(IWI_MAX_IBSSNODE_NBYTE * NBBY)
};

struct iwi_softc {
	struct ieee80211com	sc_ic;
	int			(*sc_newstate)(struct ieee80211com *,
				    enum ieee80211_state, int);
	void			(*sc_node_free)(struct ieee80211_node *);
	device_t		sc_dev;

	struct iwi_firmware	fw;
	uint32_t		flags;
#define IWI_FLAG_FW_CACHED	0x01
#define IWI_FLAG_FW_INITED	0x02
#define IWI_FLAG_FW_WARNED	0x04
#define IWI_FLAG_SCANNING	0x08
#define IWI_FLAG_EXIT		0x10	/* detaching */
#define IWI_FLAG_RESET		0x20	/* need to reset firmware */
#define IWI_FLAG_MONITOR	0x40	/* monitor thread was created */

	struct iwi_cmd_ring	cmdq;
	struct iwi_tx_ring	txq[WME_NUM_AC];
	struct iwi_rx_ring	rxq;

	struct resource		*irq;
	struct resource		*mem;
	bus_space_tag_t		sc_st;
	bus_space_handle_t	sc_sh;
	void 			*sc_ih;
	int			mem_rid;
	int			irq_rid;

	int			antenna;
	int			dwelltime;
	int			bluetooth;

	int			sc_tx_timer;

	struct bpf_if		*sc_drvbpf;

	union {
		struct iwi_rx_radiotap_header th;
		uint8_t	pad[64];
	}			sc_rxtapu;
#define sc_rxtap	sc_rxtapu.th
	int			sc_rxtap_len;

	union {
		struct iwi_tx_radiotap_header th;
		uint8_t	pad[64];
	}			sc_txtapu;
#define sc_txtap	sc_txtapu.th
	int			sc_txtap_len;

	uint8_t			sc_ibss_node[IWI_MAX_IBSSNODE_NBYTE];

	struct sysctl_ctx_list	sysctl_ctx;
	struct sysctl_oid	*sysctl_tree;
	struct thread		*sc_fw_monitor;
};

#define SIOCSLOADFW	 _IOW('i', 137, struct ifreq)
#define SIOCSKILLFW	 _IOW('i', 138, struct ifreq)

#define IWI_FW_INITIALIZED(sc)	(sc + 1)
#define IWI_FW_CMD_ACKED(sc)	(sc + 2)
#define IWI_FW_WAKE_MONITOR(sc)	(sc + 3)
#define IWI_FW_EXIT_MONITOR(sc)	(sc + 4)
