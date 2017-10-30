double randd() { return (rand() % 1000000) / 1000000. + .0000005; }

double cellgfxdist(cell *c, int i) {
  return nontruncated ? tessf : (c->type == 6 && (i&1)) ? hexhexdist : crossf;
  }

transmatrix cellrelmatrix(cell *c, int i) {
  double d = cellgfxdist(c, i);
  return ddspin(c, i) * xpush(d) * iddspin(c->mov[i], c->spin(i), euclid ? 0 : S42);
  }

hyperpoint randomPointIn(int t) {
  while(true) {
    hyperpoint h = spin(2*M_PI*(randd()-.5)/t) * tC0(xpush(asinh(randd())));
    double d =
      nontruncated ? tessf : t == 6 ? hexhexdist : crossf;
    if(hdist0(h) < hdist0(xpush(-d) * h))
      return spin(2*M_PI/t * (rand() % t)) * h;
    }
  }

struct snowball {
  transmatrix T;
  transmatrix global;
  snowball *prev;
  snowball *next;
  double phase;
  snowball(int t) { T = rgpushxto0(randomPointIn(t)); phase = randd(); }
  };                                                                

struct blizzardcell {
  cell *c;
  int frame;
  int tmp;
  transmatrix *gm;
  char wmap;
  int inward, outward, ward;
  int qty[7];
  vector<snowball*> inorder, outorder;
  int inid, outid;
  ~blizzardcell() { for(auto i: inorder) delete i; }
  };

map<cell*, blizzardcell> blizzardcells;

vector<blizzardcell*> bcells;

int N;

blizzardcell* getbcell(cell *c) {
  int i = c->aitmp;
  if(i<0 || i >= N) return NULL;
  if(bcells[i]->c != c) return NULL;
  return bcells[i];
  }

void drawBlizzards() {
  poly_outline = OUTLINE_NONE;
  auto it = blizzardcells.begin();
  bcells.clear();
  while(it != blizzardcells.end()) 
    if(it->second.frame != frameid || !gmatrix.count(it->first))
      it = blizzardcells.erase(it);
      else {
        it->second.c = it->first;
        bcells.push_back(&it->second);
        it++;
        }
  N = size(bcells);
  for(int i=0; i<N; i++) {
    auto& bc = *bcells[i];
    bc.tmp = bc.c->aitmp,
    bc.c->aitmp = i;
    bc.gm = &gmatrix[bc.c];
    bc.wmap = windmap::at(bc.c);
    }

  for(int i=0; i<N; i++) {
    auto& bc = *bcells[i];
    cell *c = bc.c;
    bc.inward = bc.outward = 0;
    for(int i=0; i<c->type; i++) {
      int& qty = bc.qty[i];
      qty = 0;
      cell *c2 = c->mov[i];
      if(!c2) continue;
      auto bc2 = getbcell(c2);
      if(!bc2) continue;
      int z = (bc2->wmap - bc.wmap) & 255;
      if(z >= windmap::NOWINDBELOW && z < windmap::NOWINDFROM)
        bc.outward += qty = z / 8;
      z = (-z) & 255;
      if(z >= windmap::NOWINDBELOW && z < windmap::NOWINDFROM)
        bc.inward += z / 8, qty = -z/8;
      }
    bc.ward = max(bc.inward, bc.outward);
    while(size(bc.inorder) < bc.ward) {
      auto sb = new snowball(c->type);
      bc.inorder.push_back(sb);
      bc.outorder.push_back(sb);
      }
    for(auto& sb: bc.inorder) sb->prev = sb->next = NULL;
    bc.inid = 0;
    }
  
  double at = (ticks % 250) / 250.0;

  for(int i=0; i<N; i++) {
    auto& bc = *bcells[i];
    for(auto sb: bc.inorder)
      sb->global = (*bc.gm) * sb->T;
    }

  for(int i=0; i<N; i++) {
    auto& bc = *bcells[i];
    cell *c = bc.c;
    
    bc.outid = 0;
    
    for(int d=0; d<c->type; d++) for(int k=0; k<bc.qty[d]; k++) {
      auto& bc2 = *getbcell(c->mov[d]);
      auto& sball = *bc.outorder[bc.outid++];
      auto& sball2 = *bc2.inorder[bc2.inid++];
      sball.next = &sball2;
      sball2.prev = &sball;
      
      hyperpoint t = inverse(sball.global) * tC0(sball2.global);
      double at0 = at + sball.phase;
      if(at0>1) at0 -= 1;
      transmatrix tpartial = sball.global * rspintox(t) * xpush(hdist0(t) * at0);
      
      if(wmascii || wmblack)
        queuechr(tpartial, .2, '.', 0xFFFFFF);
      else
        queuepoly(tpartial, shSnowball, 0xFFFFFF80);
      }
    }

  for(int ii=0; ii<N; ii++) {
    auto& bc = *bcells[ii];
    if(isNeighbor(bc.c, mouseover)) {
      if(againstWind(mouseover, bc.c))
        queuepoly(*bc.gm, shHeptaMarker, 0x00C00040);
      if(againstWind(bc.c, mouseover))
        queuepoly(*bc.gm, shHeptaMarker, 0xC0000040);
      }
    int B = size(bc.outorder);
    if(B<2) continue;
    int i = rand() % B;
    int j = rand() % (B-1);
    if(i==j) j++;
    
    if(1) {
      auto& sb1 = *bc.outorder[i];
      auto& sb2 = *bc.outorder[j];
      
      double swapcost = 0;
      if(sb1.next) swapcost -= hdist(tC0(sb1.global), tC0(sb1.next->global));
      if(sb2.next) swapcost -= hdist(tC0(sb2.global), tC0(sb2.next->global));
      if(sb1.next) swapcost += hdist(tC0(sb2.global), tC0(sb1.next->global));
      if(sb2.next) swapcost += hdist(tC0(sb1.global), tC0(sb2.next->global));
      if(swapcost < 0) {
        swap(bc.outorder[i], bc.outorder[j]);
        swap(sb1.next, sb2.next);
        if(sb1.next) sb1.next->prev = &sb1;
        if(sb2.next) sb2.next->prev = &sb2;
        }
      }
    
    if(1) {
      auto& sb1 = *bc.inorder[i];
      auto& sb2 = *bc.inorder[j];
      
      double swapcost = 0;
      if(sb1.prev) swapcost -= hdist(tC0(sb1.global), tC0(sb1.prev->global));
      if(sb2.prev) swapcost -= hdist(tC0(sb2.global), tC0(sb2.prev->global));
      if(sb1.prev) swapcost += hdist(tC0(sb2.global), tC0(sb1.prev->global));
      if(sb2.prev) swapcost += hdist(tC0(sb1.global), tC0(sb2.prev->global));
      if(swapcost < 0) {
        swap(bc.inorder[i], bc.inorder[j]);
        swap(sb1.prev, sb2.prev);
        if(sb1.prev) sb1.prev->next = &sb1;
        if(sb2.prev) sb2.prev->next = &sb2;
        }
      }
    
    auto& sbp = *bc.inorder[i];
    if(sbp.next && sbp.prev) {
      double p1 = sbp.next->phase;
      double p2 = sbp.prev->phase;
      double d = p2-p1;
      if(d<=.5) d+=1;
      if(d>=.5) d-=1;
      sbp.phase = p1 + d/2;
      if(sbp.phase >= 1) sbp.phase -= 1;
      if(sbp.phase < 0) sbp.phase += 1;
      }
    }

  for(auto bc: bcells)
    bc->c->aitmp = bc->tmp;
  }
       
vector<cell*> arrowtraps;

void drawArrowTraps() {
  for(cell *c: arrowtraps) {
    auto r = traplimits(c);
    
    try {
      transmatrix& t0 = gmatrix.at(r[0]);
      transmatrix& t1 = gmatrix.at(r[4]);

      queueline(tC0(t0), tC0(t1), 0xFF0000FF, 4, PPR_ITEM);
      if((c->wparam & 7) == 3 && !shmup::on) {
//        queueline(t0 * randomPointIn(r[0]->type), t1 * randomPointIn(r[1]->type), 0xFFFFFFFF, 4, PPR_ITEM);
        int tt = ticks % 401;
        
        for(int u=0; u<2; u++) {
          transmatrix& tu = u ? t0 : t1;
          transmatrix& tv = u ? t1 : t0;
          hyperpoint trel = inverse(tu) * tC0(tv);
          transmatrix tpartial = tu * rspintox(trel) * xpush(hdist0(trel) * tt / 401.0);
          queuepoly(tpartial * ypush(.05), shTrapArrow, 0xFFFFFFFF);
          }
        }
      }
    catch(out_of_range) {}
    }
  }

auto ccm_blizzard = addHook(clearmemory, 0, [] () {
  arrowtraps.clear();
  blizzardcells.clear();
  bcells.clear();
  });
