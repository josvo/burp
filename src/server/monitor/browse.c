#include "../../burp.h"
#include "../../alloc.h"
#include "../../bu.h"
#include "../../cmd.h"
#include "../../cstat.h"
#include "../../prepend.h"
#include "../../sbuf.h"
#include "../list.h"
#include "../manio.h"
#include "cache.h"
#include "json_output.h"

static int do_browse_manifest(struct asfd *srfd,
	struct manio *manio, struct sbuf *sb, const char *browse)
{
	int ret=-1;
	int ars=0;
	//char ls[1024]="";
	//struct cntr cntr;
	size_t blen=0;
	char *last_bd_match=NULL;
	if(browse) blen=strlen(browse);
	while(1)
	{
		int r;
		sbuf_free_content(sb);
		if((ars=manio_read(manio, sb)))
		{
			if(ars<0) goto end;
			// ars==1 means it ended ok.
			break;
		}

		if(manio->protocol==PROTO_2 && sb->endfile.buf)
			continue;

		if(sb->path.cmd!=CMD_DIRECTORY
		  && sb->path.cmd!=CMD_FILE
		  && sb->path.cmd!=CMD_ENC_FILE
		  && sb->path.cmd!=CMD_EFS_FILE
		  && sb->path.cmd!=CMD_SPECIAL
		  && !cmd_is_link(sb->path.cmd))
			continue;

		if((r=check_browsedir(browse, sb, blen, &last_bd_match))<0)
			goto end;
		if(!r) continue;

		if(json_from_statp(sb->path.buf, &sb->statp)) goto end;
	}

	ret=0;
end:
	free_w(&last_bd_match);
	return ret;
}

static int browse_manifest_start(struct asfd *srfd, struct cstat *cstat,
	struct bu *bu, const char *browse, struct conf **confs)
{
	int ret=-1;
	char *manifest=NULL;
	struct sbuf *sb=NULL;
	struct manio *manio=NULL;

	if(!(manifest=prepend_s(bu->path,
		cstat->protocol==PROTO_1?"manifest.gz":"manifest"))
	  || !(manio=manio_open(manifest, "rb", cstat->protocol))
	  || !(sb=sbuf_alloc(cstat->protocol)))
		goto end;
	if(get_int(confs[OPT_MONITOR_BROWSE_CACHE]))
		ret=cache_load(srfd, manio, sb, cstat, bu);
	else
		ret=do_browse_manifest(srfd, manio, sb, browse);
end:
	free_w(&manifest);
	manio_close(&manio);
	sbuf_free(&sb);
	return ret;
}

int browse_manifest(struct asfd *srfd, struct cstat *cstat,
	struct bu *bu, const char *browse, struct conf **confs)
{
	if(get_int(confs[OPT_MONITOR_BROWSE_CACHE]))
	{
		if(!cache_loaded(cstat, bu)
		  && browse_manifest_start(srfd, cstat, bu, browse, confs))
			return -1;
		return cache_lookup(browse);
	}
	return browse_manifest_start(srfd, cstat, bu, browse, confs);
}
