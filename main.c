#include"src/spng.h"
#include<stddef.h>
#include<stdio.h>
#include<limits.h>
#include"src/schrift.h"
#include<ctype.h>

#define CLAMP(a) (a) > 255 ? 255 : (a) < 0 ? 0 : (a)

char* readFile(char *fn, size_t *len){
	FILE *f = fopen(fn, "rb");
	if(f && !fseek(f, 0, SEEK_END)){
		long int length = ftell(f);
		if(length != -1){
			rewind(f);
			char *buffer = malloc(length + 1);
			if(buffer && fread(buffer, 1, length, f) == length) {
				buffer[length] = 0;
				fclose(f);
				*len = (size_t)length;
				return buffer;
			}
			free(buffer);
		}
		fclose(f);
	}

	printf("failed to read file: \"%s\"\n", fn);
	*len = 0;
	return NULL;
}

typedef unsigned char uchar;
uchar *render_char(char cp, size_t *width, size_t *height){
	SFT sft = {
		.xScale = 32,
		.yScale = 32,
		.flags = SFT_DOWNWARD_Y,
		.font = sft_loadfile("./NotoSansMono-Bold.ttf")
	};

	SFT_Glyph gid;
	if (sft_lookup(&sft, cp, &gid) < 0)
		return NULL;

	SFT_GMetrics mtx;
	if (sft_gmetrics(&sft, gid, &mtx) < 0)
		return NULL;

	SFT_LMetrics lmtx;
	if(sft_lmetrics(&sft, &lmtx) < 0)
		return NULL;


	SFT_Image img = {
		.width  = (mtx.minWidth + 3) & ~3,
		.height = mtx.minHeight,
	};

	char *pixels = malloc(img.width * img.height * sizeof(char));
	img.pixels = pixels;
	if (sft_render(&sft, gid, img) < 0){
		free(pixels);
		return NULL;
	}

	ssize_t ow = mtx.advanceWidth, oh = lmtx.ascender - lmtx.descender + lmtx.lineGap + 1;
	char *outb = calloc(sizeof(char), ow * oh);
	//printf("%f %f  %d %d %d %d\n", mtx.leftSideBearing, oh + mtx.yOffset + lmtx.descender, img.width, img.height, ow, oh);
	for(int i = 0; i < img.height; i++){
		for(int j = 0; j < img.width; j++){
			int y = i + oh + mtx.yOffset + lmtx.descender;
			int x = j + mtx.leftSideBearing;
			outb[x + y*ow] = pixels[j + i*img.width];
		}
	}

	free(pixels);
	sft_freefont(sft.font);

	*height = oh;
	*width = ow;
	return outb;
}


uchar rgbcolors[][3] = {
	{0, 0, 0}, {170, 0, 0}, {0, 170, 0}, {170, 85, 0}, {0, 0, 170}, {170, 0, 170}, {0, 170, 170}, {170, 170, 170}, {85, 85, 85}, {255, 85, 85}, {85, 255, 85}, {255, 255, 85}, {85, 85, 255}, {255, 85, 255}, {85, 255, 255}, {255, 255, 255}
};

char *fgcolors[] = {
	"30", "31", "32", "33", "34", "35", "36", "37", "90", "91", "92", "93", "94", "95", "96", "97"
};

char *bgcolors[] = {
	"40", "41", "42", "43", "44", "45", "46", "47", "100", "101", "102", "103", "104", "105", "106", "107"
};

int closest_color(uchar r, uchar g, uchar b){
	int min = 0;
	int dist = INT_MAX;
	for(int i = 0; i < 16; i++){
		int dr = rgbcolors[i][0] - r, 
		    dg = rgbcolors[i][1] - g, 
		    db = rgbcolors[i][2] - b;

		int d = dr*dr + dg*dg + db*db;
		if(d < dist){
			dist = d;
			min = i;
		}
	}

	return min;
}

char* parse_png(char *path, size_t *width, size_t *height){
	spng_ctx *ctx = spng_ctx_new(0);

	size_t size;
	char *png = readFile(path, &size);
	
	size_t out_size;
	spng_set_png_buffer(ctx, png, size);
	spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &out_size);

	char *buf = malloc(out_size);
	spng_decode_image(ctx, buf, out_size, SPNG_FMT_RGB8, 0);

	struct spng_ihdr ihdr;
	spng_get_ihdr(ctx, &ihdr);

	*width = ihdr.width;
	*height = ihdr.height;

	spng_ctx_free(ctx);
	return buf;
}

void sample_png(
		uchar *png,
		size_t w, size_t h,
		size_t tw, size_t th,
		size_t x, size_t y,
		uchar *col
){
	float r = 0.0, g = 0.0, b = 0.0;
	float pw = (float)w / tw, ph = (float)h / th;
	float px = pw*x, py = ph*y;
	float a = pw * ph;
	
	
	int sx = px, sy = py;
	int ex = pw + px, ey = ph + py;
	//printf("\e[0m%d %d %d %d\n", sx, sy, ex, ey);
	//printf("\e[0m%f %f %f %f\n", px, py, px + pw, py + ph);

	float sw = 1.0-(px-sx), sh = 1.0-(py-sy);
	float ew = pw + px - ex, eh = ph + py - ey;

	if(sx == ex){
		sw = pw;
		ew = 0.0;
	}

	if(sy == ey){
		sh = ph;
		eh = 0.0;
	}

	// top left section
	r += sw * sh * png[3*sx + 3*w*sy];
	g += sw * sh * png[3*sx + 3*w*sy + 1];
	b += sw * sh * png[3*sx + 3*w*sy + 2];

	//bottom left
	r += sw * eh * png[3*sx + 3*w*ey];
	g += sw * eh * png[3*sx + 3*w*ey + 1];
	b += sw * eh * png[3*sx + 3*w*ey + 2];
	
	//bottom right
	r += ew * eh * png[3*ex + 3*w*ey];
	g += ew * eh * png[3*ex + 3*w*ey + 1];
	b += ew * eh * png[3*ex + 3*w*ey + 2];
	
	//top right
	r += ew * sh * png[3*ex + 3*w*sy];
	g += ew * sh * png[3*ex + 3*w*sy + 1];
	b += ew * sh * png[3*ex + 3*w*sy + 2];

	// top section
	for(int i = sx + 1; i < ex; i++){
		r += sh * png[3*i + 3*w*sy];
		g += sh * png[3*i + 3*w*sy + 1];
		b += sh * png[3*i + 3*w*sy + 2];
	}

	//left section
	for(int i = sy + 1; i < ey; i++){
		r += sw * png[3*sx + 3*w*i];
		g += sw * png[3*sx + 3*w*i + 1];
		b += sw * png[3*sx + 3*w*i + 2];
	}

	//right section
	for(int i = sy + 1; i < ey; i++){
		r += ew * png[3*ex + 3*w*i];
		g += ew * png[3*ex + 3*w*i + 1];
		b += ew * png[3*ex + 3*w*i + 2];
	}

	//bottom section
	for(int i = sx + 1; i < ex; i++){
		r += eh * png[3*i + 3*w*ey];
		g += eh * png[3*i + 3*w*ey + 1];
		b += eh * png[3*i + 3*w*ey + 2];
	}

	//middle section
	for(int i = sx + 1; i < ex; i++){
		for(int j = sy + 1; j < ey; j++){
			r += png[3*i + 3*w*j];
			g += png[3*i + 3*w*j + 1];
			b += png[3*i + 3*w*j + 2];
		}
	}

	col[0] = r / a;
	col[1] = g / a;
	col[2] = b / a;
}

uchar* resize_png(uchar *png, size_t w, size_t h, size_t tw, size_t th){
	uchar *ret = malloc(tw*th*3*sizeof(uchar));

	for(size_t i = 0; i < th; i++)
		for(size_t j = 0; j < tw; j++)
			sample_png(png, w, h, tw, th, j, i, ret + 3*j + 3*tw*i);

	return ret;
}

float urand(){
	return 2.0*((float)rand()/RAND_MAX - 0.5);
}

char best_char(uchar *png, size_t w, size_t h, size_t x, size_t y, uchar *err){
	uchar best = ' ';
	float best_err = 1.0/0.0;
	float bfgr, bfgg, bfgb;
	float bbgr, bbgg, bbgb;
	float bbgp, bfgp;
	float bber, bbeg, bbeb;
	float bfer, bfeg, bfeb;

	for(uchar q = 32; q < 128; q++){
		size_t cw, ch;
		uchar *cha = render_char(q, &cw, &ch);

		if(cha == NULL || !isprint(q))
			continue;

		//calc average
		float bgr = 0, bgb = 0, bgg = 0;
		float bgp = 0;

		float fgr = 0, fgb = 0, fgg = 0;
		float fgp = 0;
		for(int i = 0; i < ch; i++){
			for(int j = 0; j < cw; j++){
				int v = cha[i*cw + j];
			       
				uchar r = png[(i + y)*w*3 + (j + x)*3],
				    g = png[(i + y)*w*3 + (j + x)*3 + 1],
				    b = png[(i + y)*w*3 + (j + x)*3 + 2];

				if(v == 0){
					bgp += 1;
					bgr += r;
					bgb += b;
					bgg += g;
				}else{
					fgp += 1;
					fgr += r;
					fgg += g;
					fgb += b;
				}
				


			}
		}

		bgr /= bgp;
		bgg /= bgp;
		bgb /= bgp;

		fgr /= fgp;
		fgg /= fgp;
		fgb /= fgp;

		//calc mean square error
		float ber = 0, beb = 0, beg = 0;
		float fer = 0, feb = 0, feg = 0;
		for(int i = 0; i < ch; i++){
			for(int j = 0; j < cw; j++){
				int v = cha[i*cw + j];
			       
				uchar r = png[(i + y)*w*3 + (j + x)*3],
				    g = png[(i + y)*w*3 + (j + x)*3 + 1],
				    b = png[(i + y)*w*3 + (j + x)*3 + 2];

				float bar = bgr - r,
				      bag = bgg - g,
				      bab = bgb - b;

				float far = fgr - r,
				      fag = fgg - g,
				      fab = fgb - b;

				if(v == 0){
					ber += bar*bar;
					beg += bag*bag;
					beb += bab*bab;
				} else {
					fer += far*far;
					feg += fag*fag;
					feb += fab*fab;
				}
			}
		}

		ber /= bgp;
		beg /= bgp;
		beb /= bgp;

		fer /= fgp;
		feg /= fgp;
		feb /= fgp;

		float err = ber*ber + beg*beg + beb*beb + fer*fer + feg*feg + feb*feb;

		if(err < best_err){
			best = q;
			best_err = err;

			bbgr = bgr;
			bbgg = bgg;
			bbgb = bgb;

			bfgr = fgr;
			bfgg = fgg;
			bfgb = fgb;

			bbgp = bgp;
			bfgp = fgp;
	
			bber = sqrt(ber);
			bbeg = sqrt(beg);
			bbeb = sqrt(beb);

			bfer = sqrt(fer);
			bfeg = sqrt(feg);
			bfeb = sqrt(feb);
		}

		free(cha);

	}

	float fgr = bfgr,
	      fgg = bfgg,
	      fgb = bfgb;

	int col = closest_color(CLAMP(fgr), CLAMP(fgg), CLAMP(fgb));

	float ar = (fgr - rgbcolors[col][0] + urand()*bfer) * bfgp / bbgp,
	      ag = (fgg - rgbcolors[col][1] + urand()*bfeg) * bfgp / bbgp,
	      ab = (fgb - rgbcolors[col][2] + urand()*bfeb) * bfgp / bbgp;

	float bgr = bbgr + ar + err[0],
	      bgg = bbgg + ag + err[1],
	      bgb = bbgb + ab + err[2];

	int col2 = closest_color(CLAMP(bgr), CLAMP(bgg), CLAMP(bgb));

	float br = (bgr - rgbcolors[col2][0] + urand()*bber),
	      bg = (bgg - rgbcolors[col2][1] + urand()*bbeg),
	      bb = (bgb - rgbcolors[col2][2] + urand()*bbeb);

	err[0] = CLAMP(br);
	err[1] = CLAMP(bg);
	err[2] = CLAMP(bb);

	printf("\e[1;%s;%sm%c",
			bgcolors[col2],
			fgcolors[col], best);
}

float bayer_[16] = {
	0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
	12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
	3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
	15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
};

int main(int c, char **v){

		size_t cw, ch;
		uchar *cha = render_char('A', &cw, &ch);
		free(cha);
	//for(uchar q = 32; q < 128; q++){
		size_t w, h;
		uchar *png = parse_png(v[1], &w, &h);
		if(png == NULL || w == 0 || h == 0)
			return 1;

		
#define MAX(a, b) ((a) > (b) ? (a) : (b))

		size_t mw = w;//MAX(w, h);


		size_t ma = 160;
		size_t tw = w*ma*cw/mw, th = h*ma*cw/mw;

		printf("%d %d\n", tw, th);
		printf("%d %d\n", cw, ch);
		uchar *rpng = resize_png(png, w, h, tw, th);

		uchar *err = malloc(3 * (tw/cw + 1) * sizeof(uchar));
		err[0] = err[1] = err[2] = 0;


		for(int i = 0; (i+ch) <= th; i += ch){
			for(int j = 0; (j+cw) <= tw; j += cw){
				uchar *ep = err + 3*(j/cw);
				best_char(rpng, tw, th, j, i, ep);

				ep[0] *= 0.5;
				ep[1] *= 0.5;
				ep[2] *= 0.5;

				ep[3] = ep[0];
				ep[4] = ep[1];
				ep[5] = ep[2];


				

			}
			printf("\n");
		}
		
		/*
		for(int i = 0; i+1 < th; i++){
			for(int j = 0; j+1 < tw; j++){
				uchar *r = rpng + i*tw*3 + j*3,
				    *g = rpng + i*tw*3 + j*3 + 1,
				    *b = rpng + i*tw*3 + j*3 + 2;
					
				int col = closest_color(*r, *g, *b);

				float ar = *r - rgbcolors[col][0],
				      ag = *g - rgbcolors[col][1],
				      ab = *b - rgbcolors[col][2];

				//float bv = 0.5*((float)rand() / RAND_MAX);
				//float bv = 0.5*bayer_[(j % 4) + 4*(i % 4)];
				float bv = 0.2;

#define CLAMP(a) (a) > 255 ? 255 : (a) < 0 ? 0 : (a)

				r[w*3] = CLAMP(r[w*3] + bv*ar);
				r[3] = CLAMP(r[3] + bv*ar);
				g[w*3] = CLAMP(g[w*3] + bv*ag);
				g[3] = CLAMP(g[3] + bv*ag);
				b[w*3] = CLAMP(b[w*3] + bv*ab);
				b[3] = CLAMP(b[3] + bv*ab);


				printf("\e[%s;%sm#",
						bgcolors[col],
						fgcolors[0]);
			}
			printf("\e[0m\n");
		}
		printf("\n");
		free(png);
		*/
	//}
}

