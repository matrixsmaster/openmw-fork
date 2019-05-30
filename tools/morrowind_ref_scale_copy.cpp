#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <map>

using namespace std;

typedef struct {
	int off;
	int len;
	uint8_t* data;
} datarec_t;

typedef map<uint32_t,pair<string,float> > refmap;
vector<refmap> cellvec;

datarec_t readrec(const char* typ, uint8_t* buf, int buflen, int fixoff)
{
	datarec_t ret = {0};
	ret.off = -1;

    if (buflen < 0) return ret;
    
	uint8_t* ptr = (uint8_t*)memmem(buf,buflen,typ,strlen(typ));
	if (!ptr) return ret;
	
	ptr += strlen(typ);
	ret.len = *((uint32_t*)ptr);
	//printf("%s record of length %u\n",typ,ret.len);
	
	if (ret.len > 0) ret.data = ptr + 4 + fixoff;
	
	ret.off = ptr - buf;
	return ret;
}

int scan(bool write, char* in, char* out)
{
	FILE* fi = fopen(in,"rb");
	assert(fi);
	
	fseek(fi,0,SEEK_END);
	size_t inlen = ftell(fi);
	fseek(fi,0,SEEK_SET);

	uint8_t* inp = new uint8_t[inlen];
	assert(fread(inp,inlen,1,fi));
	fclose(fi);
	
	uint8_t* pos = inp;
    auto curcell = cellvec.begin();
	for (;;) {
        refmap mcell;
        if (write && curcell != cellvec.end()) {
            mcell = *curcell;
            printf("Map of %lu loaded\n",mcell.size());
            ++curcell;
        }
        
        int rem = inlen - (uint32_t)(pos-inp);
		datarec_t cp = readrec("CELL",pos,rem,8);
		if (cp.off < 0) break;
        pos += cp.off;
		assert(cp.data);

        printf("Cell length %d @ 0x%08X\n", cp.len, (uint32_t)(pos-inp));

        datarec_t data = readrec("DATA",cp.data,cp.len,0);
		datarec_t name = readrec("NAME",cp.data,cp.len,0);
		assert(name.len || data.len);

        int ioff = 0;
        if (data.off < name.off) {
            printf("Unnamed cell\n");
            ioff = data.off;
        } else {
            printf("Cell name: %s\n", name.data);
            ioff = name.off + name.len;
        }

        for (uint32_t nidx = 0;;) {
            datarec_t frmr = readrec("FRMR",cp.data+ioff,cp.len-ioff,0);
            if (frmr.off < 0) break;
            ioff += frmr.off + frmr.len;

            uint32_t idx = *((uint32_t*)frmr.data);
            datarec_t fdata = readrec("DATA",cp.data+ioff,cp.len-ioff,0);
            datarec_t fxscl = readrec("XSCL",cp.data+ioff,cp.len-ioff,0);
            datarec_t fname = readrec("NAME",cp.data+ioff,cp.len-ioff,0);
            datarec_t fdele = readrec("DELE",cp.data+ioff,cp.len-ioff,0);
            datarec_t nfrmr = readrec("FRMR",cp.data+ioff,cp.len-ioff,0);
            assert(fname.len || fdata.len || fxscl.len || fdele.len);

#if 0
            printf("name off: %d\n",fname.off);
            printf("xscl off: %d\n",fxscl.off);
            printf("data off: %d\n",fdata.off);
            printf("dele off: %d\n",fdele.off);
            printf("frmr off: %d\n",nfrmr.off);
            printf("remaining %d\n",cp.len-ioff);
#endif

            if (nfrmr.off < 0) nfrmr.off = cp.len; // next record is out of bounds

            string mname;
            if (fname.len && fname.off < nfrmr.off) {
                printf("\tRef #%u: %s\n", idx, fname.data);
                mname = (char*)fname.data;

            } else if (fdata.len && fdata.off < nfrmr.off)
                printf("\tUnnamed ref #%u\n",idx);
            
            if (fdele.len && fdele.off < nfrmr.off) {
                printf("\t\tDeleted ref #%u\n", idx);
                ioff += fdele.off + fdele.len;
                continue;
            }
            
            if (fxscl.len && fxscl.off < fdata.off) {
                float* scale = (float*)fxscl.data;
                printf("\t\tScale = %f\n", *scale);

                if (!write)
                    mcell[nidx] = pair<string,float>(mname,*scale);
                    
                else if (mcell.count(nidx)) {
                        printf("\t\t\tIndex found, checking name...\n");
                        
                        if (mcell.at(nidx).first == mname) {
                            printf("\t\t\tName check PASSED\n");
                            printf("\t\t\tFixing scale to %f\n",mcell.at(nidx).second);
                            *scale = mcell.at(nidx).second;
                        } else
                            printf("\t\t\tName check FAILED (%s expected)\n",mcell.at(nidx).first.c_str());
                }
                ++nidx;
            }

            //printf("data @0x%08X, len %d\n",fdata.off+ioff+(uint32_t)(pos-inp)+8,fdata.len);
            if (fdata.off >= 0)
                ioff += fdata.off + fdata.len + 4;
            else if (fdele.off >= 0)
                ioff += fdele.off + fdele.len + 4;
            else if (fname.off >= 0)
                ioff += fname.off + fname.len + 4;
        }

        pos += cp.len;
        //break;

        if (!write) cellvec.push_back(mcell);
	}

    if (write) {
        FILE* fo = fopen(out,"wb");
        assert(fo);

        assert(fwrite(inp,inlen,1,fo));
        fclose(fo);
    }

    delete[] inp;
	
	return 0;
}

int main(int argc, char* argv[])
{
    assert(argc > 3);

    printf("read = %d\n", scan(false, argv[1], NULL));
    printf("write = %d\n", scan(true, argv[2], argv[3]));
    
    return 0;
}
