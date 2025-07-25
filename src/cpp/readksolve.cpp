#include "readksolve.h"
#include "parsemoves.h"
#include <iostream>
int nocorners, nocenters, noedges, ignoreori, distinguishall;
set<string> omitsets, omitperms, omitoris, setsmustexist;
static int lineno;
void inerror(const string s, const string x = "") {
  if (lineno)
    cerr << lineno << ": ";
  error(s, x);
}
vector<string> getline(istream *f, ull &checksum) {
  string s;
  int c;
  lineno++;
  while (1) {
    s.clear();
    while (1) {
      c = f->get();
      if (c != EOF)
        checksum = 31 * checksum + c;
      if (c == EOF || c == 10 || c == 13) {
        if (c == EOF || s.size() > 0)
          break;
        else {
          if (c == 10)
            lineno++;
          continue;
        }
      }
      s.push_back((char)c);
    }
    vector<string> toks;
    if (s.size() == 0) {
      curline = s;
      return toks;
    }
    if (verbose > 2)
      cout << ">> " << s << endl;
    if (s[0] == '#') {
      lineno++;
      continue;
    }
    string tok;
    for (int i = 0; i < (int)s.size(); i++) {
      if (s[i] <= ' ') {
        if (tok.size() > 0) {
          toks.push_back(tok);
          tok.clear();
        }
      } else {
        tok.push_back(s[i]);
      }
    }
    if (tok.size() > 0)
      toks.push_back(tok);
    if (toks.size() == 0) {
      lineno++;
      continue;
    }
    curline = s;
    return toks;
  }
}
void expect(const vector<string> &toks, int cnt) {
  if (cnt != (int)toks.size())
    inerror("! wrong number of tokens on line");
}
// must be a number under 256.
int getnumber(int minval, const string &s) {
  int r = 0;
  for (int i = 0; i < (int)s.size(); i++) {
    if ('0' <= s[i] && s[i] <= '9')
      r = r * 10 + s[i] - '0';
    else
      inerror("! bad character while parsing number in ", s);
  }
  if (r < minval || r > 255)
    inerror("! value out of range in ", s);
  return r;
}
// permits a ? for undefined.
int getnumberorneg(int minval, const string &s) {
  if (s == "?")
    return 255;
  int r = 0;
  for (int i = 0; i < (int)s.size(); i++) {
    if ('0' <= s[i] && s[i] <= '9')
      r = r * 10 + s[i] - '0';
    else
      inerror("! bad character while parsing number in ", s);
  }
  if (r < minval || r > 255)
    inerror("! value out of range in ", s);
  return r;
}
int isnumber(const string &s) {
  return s.size() > 0 && '0' <= s[0] && s[0] <= '9';
}
int oddperm(uchar *p, int n) {
  static uchar done[256];
  for (int i = 0; i < n; i++)
    done[i] = 0;
  int r = 0;
  for (int i = 0; i < n; i++)
    if (!done[i]) {
      int cnt = 1;
      done[i] = 1;
      for (int j = p[i]; !done[j]; j = p[j]) {
        done[j] = 1;
        cnt++;
      }
      if (0 == (cnt & 1))
        r++;
    }
  return r & 1;
}
int omitset(string s) {
  if (omitsets.find(s) != omitsets.end())
    return 3;
  if (s.size() >= 2) {
    if (nocorners && tolower(s[0]) == 'c' && s[1] != 0 && tolower(s[2]) == 'r')
      return 3;
    if (nocenters && tolower(s[0]) == 'c' && tolower(s[1]) == 'e')
      return 3;
    if (noedges && tolower(s[0]) == 'e' && tolower(s[1]) == 'd')
      return 3;
  }
  if (omitoris.find(s) != omitoris.end())
    return 2;
  if (omitperms.find(s) != omitperms.end())
    return 1;
  return 0;
}
/*
 *   The new style boolean indicates that permutations are zero-based
 *   and also that the orientation changes in moves are associated with
 *   the piece at that location (unlike ksolve which has a really funky
 *   way of doing moves, that make them completely distinct from states).
 */
allocsetval readposition(puzdef &pz, char typ, istream *f, ull &checksum,
                         bool new_style) {
  allocsetval r(pz, pz.id);
  int curset = -1;
  int numseq = 0;
  int ignore = 0;
  while (1) {
    vector<string> toks = getline(f, checksum);
    if (toks.size() == 0)
      inerror("! premature end while reading position");
    if (toks[0] == "End") {
      if (curset >= 0 && numseq == 0 && ignore == 0)
        inerror("! empty set def?");
      expect(toks, 1);
      ignore = 0;
      break;
    }
    if ((typ != 'm' && toks[0] == "?") || isnumber(toks[0])) {
      if (ignore == 3)
        continue;
      if (curset < 0 || numseq > 1) {
        inerror("! unexpected number sequence");
      }
      int n = pz.setdefs[curset].size;
      expect(toks, n);
      uchar *p = r.dat + pz.setdefs[curset].off + numseq * n;
      int offset = (numseq == 0 && new_style) ? 0 : 1;
      if (typ != 'm') {
        for (int i = 0; i < n; i++) {
          p[i] = getnumberorneg(offset - numseq, toks[i]);
          if (p[i] != 255) {
            p[i] += 1 - offset;
          }
        }
      } else {
        for (int i = 0; i < n; i++) {
          p[i] = getnumber(offset - numseq, toks[i]);
          if (p[i] != 255) {
            p[i] += 1 - offset;
          }
        }
      }
      if (numseq == 1 && (ignoreori || (ignore & 2)))
        for (int i = 0; i < n; i++)
          p[i] = 0;
      if (numseq == 0 && (ignore & 1) && typ == 's') {
        for (int i = 0; i < n; i++)
          p[i] = offset;
      }
      numseq++;
    } else {
      if (curset >= 0 && numseq == 0)
        inerror("! empty set def?");
      expect(toks, 1);
      ignore = omitset(toks[0]);
      if (ignore == 3)
        continue;
      curset = -1;
      for (int i = 0; i < (int)pz.setdefs.size(); i++)
        if (toks[0] == pz.setdefs[i].name) {
          curset = i;
          break;
        }
      if (curset < 0)
        inerror("Bad set name?");
      if (r.dat[pz.setdefs[curset].off])
        inerror("! redefined set name?");
      numseq = 0;
    }
  }
  for (int i = 0; i < (int)pz.setdefs.size(); i++) {
    auto &sd = pz.setdefs[i];
    uchar *p = r.dat + sd.off;
    int n = sd.size;
    ignore = omitset(sd.name);
    vector<int> cnts;
    if ((ignore & 1) == 0 && (p[0] == 0 || (typ == 's' && distinguishall))) {
      if (typ == 'S') {
        for (int j = 0; j < n; j++)
          p[j] = pz.solved.dat[sd.off + j];
      } else {
        cnts.resize(n);
        for (int j = 0; j < n; j++) {
          p[j] = j; // identity perm
          cnts[j]++;
        }
        if (typ == 's') {
          sd.psum = n * (n - 1) / 2;
          sd.cnts = cnts;
        }
      }
    } else {
      if (typ == 's') {
        for (int j = 0; j < n; j++) {
          if (p[j] > (int)cnts.size())
            cnts.resize(p[j]);
          cnts[p[j] - 1]++;
        }
        for (int j = 0; j < (int)cnts.size(); j++)
          if (cnts[j] == 0) {
            // We have some elements that are not contiguous.  We will
            // build and populate the pack and unpack arrays for input
            // and output.
            sd.pack.resize(cnts.size());
            for (int k = 0; k < (int)cnts.size(); k++)
              if (cnts[k] != 0) {
                sd.pack[k] = sd.unpack.size();
                sd.unpack.push_back(k);
              }
            break;
          }
        cnts.clear();
      }
      int sum = 0;
      if (typ != 'm' && sd.pack.size()) {
        for (int k = 0; k < n; k++) {
          if (p[k] > (int)sd.pack.size())
            inerror("! input value too high");
          p[k] = 1 + sd.pack[p[k] - 1];
        }
      }
      for (int j = 0; j < n; j++) {
        int v = --p[j];
        sum += v;
        if (v >= (int)cnts.size())
          cnts.resize(v + 1);
        cnts[v]++;
      }
      if (typ == 's') {
        sd.cnts = cnts;
        sd.psum = sum;
      }
      if (typ == 'S' && !(cnts == sd.cnts)) {
        inerror("! scramble position permutation doesn't match solved");
      }
      if ((int)cnts.size() != n) {
        if (typ != 's' && typ != 'S')
          inerror("! expected, but did not see, a proper permutation");
        else {
          sd.uniq = 0;
          pz.uniq = 0;
          sd.pbits = ceillog2(cnts.size());
          if (n > 64) {
            sd.dense = 0;
            pz.dense = 0;
          }
        }
      } else {
        if (typ != 'S' && oddperm(p, n))
          sd.pparity = 0;
      }
    }
    p += n;
    int s = 0;
    int checkwildo = 0;
    for (int j = 0; j < n; j++) {
      if (p[j] == 255 && typ != 'm') {
        if (sd.omod == 1) {
          p[j] = 0;
        } else {
          p[j] = 2 * sd.omod;
          if (typ == 's') {
            sd.wildo = 1;
            sd.obits = ceillog2(sd.omod + 1);
            pz.wildo = 1;
            sd.oparity = 0;
          } else {
            if (typ != 'S')
              inerror("! internal error; should be reading scramble");
            checkwildo = 1;
          }
        }
      } else if (p[j] >= sd.omod) {
        inerror("! modulo value too large");
      }
      s += p[j];
    }
    // ensure all identical pieces either are not wild orientation
    // or are all wild orientation
    if (typ == 's' && sd.wildo) {
      for (int j = 0; j < n; j++)
        for (int k = j + 1; k < n; k++)
          if (p[j - n] == p[k - n])
            if ((p[j] < sd.omod) != (p[k] < sd.omod))
              inerror("! inconsistent orientation wildcards across identical "
                      "pieces");
    }
    if (typ == 'S' && (checkwildo || sd.wildo)) {
      if (!checkwildo)
        inerror(
            "! solved state in def has ? orientation but scramble does not");
      if (!sd.wildo)
        inerror("! scramble def has ? orientation but solved state in def does "
                "not");
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          if (p[j - n] == pz.solved.dat[sd.off + k])
            if ((p[j] < sd.omod) != (pz.solved.dat[sd.off + n + k] < sd.omod))
              inerror("! inconsistent use of orientation wildcards between "
                      "solved state and scramble");
    }
    if (s % sd.omod != 0)
      sd.oparity = 0;
    if (typ == 'm' && !new_style) { // fix moves
      static uchar f[256];
      for (int j = 0; j < n; j++)
        f[j] = p[j];
      for (int j = 0; j < n; j++)
        p[j] = f[p[j - n]];
    }
  }
  return r;
}
int expandonemove(const puzdef &pd, const moove &m, vector<moove> &newmoves,
                  vector<string> &newnames) {
  stacksetval p1(pd), p2(pd);
  if (quarter && m.cost > 1)
    return -1;
  vector<allocsetval> movepowers;
  movepowers.push_back(m.pos);
  pd.assignpos(p1, m.pos);
  pd.assignpos(p2, m.pos);
  int foundzero = 0;
  for (int p = 2; p < 1000000; p++) {
    pd.mul(p1, m.pos, p2);
    if (pd.comparepos(p2, pd.id) == 0) {
      foundzero = 1;
      break;
    }
    movepowers.push_back(allocsetval(pd, p2));
    swap(p1.dat, p2.dat);
  }
  if (foundzero == 0)
    error("! could not expand the moveset; order of a move too high");
  int order = movepowers.size() + 1;
  for (int j = 0; j < (int)movepowers.size(); j++) {
    int tw = j + 1;
    if (order - tw < tw)
      tw -= order;
    moove m2 = m;
    m2.pos = movepowers[j];
    m2.cost = abs(tw);
    /* TODO: if a move with an extremely high order causes the number of
       moves to get too high, and you are using quarter move metric, then
       the make work chunks can fail or be highly suboptimal.  Uncomment
       this code as a temporary hack to fix this (but you won't be able
       to use move multiples on input anymore.)  I need to fix this in a
       better way.   -tgr
     *
    if (quarter && m2.cost > 1)
       continue ;
     * */
    m2.twist = (tw + order) % order;
    if (tw != 1) {
      string s2 = m.name;
      if (tw != -1)
        s2 += to_string(abs(tw));
      if (tw < 0)
        s2 += "'";
      newnames.push_back(s2);
      m2.name = s2;
    }
    newmoves.push_back(m2);
  }
  return order;
}
void addnewmove(puzdef &pz, moove &m, vector<string> &newnames) {
  if (isrotation(m.name)) {
    m.base = pz.baserotations.size();
    pz.baserotations.push_back(m);
    int order = expandonemove(pz, m, pz.expandedrotations, newnames);
    pz.baserotorders.push_back(order);
  } else {
    m.base = pz.basemoves.size();
    pz.basemoves.push_back(m);
    int order = expandonemove(pz, m, pz.moves, newnames);
    pz.basemoveorders.push_back(order);
  }
}
void expandalgomoves(puzdef &pd, vector<string> &newnames) {
  int needed = pd.moveseqs.size();
  if (needed == 0)
    return;
  vector<char> done(needed);
  stacksetval p1(pd), p2(pd);
  string move;
  while (needed) {
    int donethistime = 0;
    for (int i = 0; i < (int)pd.moveseqs.size(); i++) {
      if (done[i])
        continue;
      move.clear();
      pd.assignpos(p1, pd.id);
      int good = 1;
      for (auto c : pd.moveseqs[i].srcstr) {
        if (c <= ' ' || c == ',') {
          if (move.size()) {
            good = domove_or_rotation_q(pd, p1, p2, move);
            if (good == 0)
              break;
            move.clear();
          }
        } else
          move.push_back(c);
      }
      if (good && move.size())
        good = domove_or_rotation_q(pd, p1, p2, move);
      if (good) {
        moove m(pd, pd.id);
        m.name = pd.moveseqs[i].name;
        m.pos = allocsetval(pd, p1);
        m.cost = 1;
        m.twist = 1;
        m.isalias = pd.moveseqs[i].isalias;
        addnewmove(pd, m, newnames);
        done[i] = 1;
        donethistime++;
        needed--;
      }
    }
    if (donethistime == 0) {
      cerr << "Unexpanded:" << endl;
      for (int i = 0; i < (int)pd.moveseqs.size(); i++)
        if (!done[i])
          cerr << " " << pd.moveseqs[i].name << " = " << pd.moveseqs[i].srcstr
               << endl;
      error("! could not expand all move sequences");
    }
  }
}
puzdef readdef(istream *f) {
  vector<string> newnames;
  curline.clear();
  puzdef pz;
  int state = 0;
  ull checksum = 0;
  pz.optionssum = 0;
  lineno = 0;
  int ignore = 0;
  while (1) {
    vector<string> toks = getline(f, checksum);
    if (toks.size() == 0)
      break;
    if (toks[0] == "Name") {
      if (state != 0)
        inerror("! Name in wrong place");
      state++;
      expect(toks, 2);
      pz.name = toks[1];
    } else if (toks[0] == "Set") {
      if (state == 0) {
        pz.name = "Unnamed";
        state++;
      }
      if (state != 1)
        inerror("! Set in wrong place");
      expect(toks, 4);
      setsmustexist.erase(toks[1]);
      ignore = omitset(toks[1]);
      if (ignore == 3)
        continue;
      setdef sd;
      sd.name = toks[1];
      sd.size = getnumber(1, toks[2]);
      sd.omod = getnumber(1, toks[3]);
      if (sd.omod > 127)
        inerror("! twsearch supports a maximum orientation size of 127");
      if (ignoreori || (ignore & 2))
        sd.omod = 1;
      sd.pparity = (sd.size == 1 ? 0 : 1);
      sd.oparity = 1;
      sd.pbits = ceillog2(sd.size);
      sd.pibits = sd.pbits;
      if (sd.wildo) {
        sd.obits = ceillog2(sd.omod + 1);
      } else {
        sd.obits = ceillog2(sd.omod);
      }
      sd.uniq = 1;
      sd.dense = 1;
      sd.off = pz.totsize;
      pz.setdefs.push_back(sd);
      pz.totsize += 2 * sd.size;
      if (gmoda[sd.omod] == 0) {
        gmoda[sd.omod] = (uchar *)calloc(4 * sd.omod, 1);
        for (int i = 0; i < 2 * sd.omod; i++)
          gmoda[sd.omod][i] = i % sd.omod;
        for (int i = 2 * sd.omod; i < 4 * sd.omod; i++)
          gmoda[sd.omod][i] = 2 * sd.omod;
      }
    } else if (toks[0] == "Illegal") {
      if (state < 2)
        inerror("! Illegal must be after solved");
      // set name, position, value, value, value, value ...
      for (int i = 3; i < (int)toks.size(); i++)
        pz.addillegal(toks[1].c_str(), getnumber(1, toks[2]),
                      getnumber(1, toks[i]));
    } else if (toks[0] == "Solved" || toks[0] == "StartState") {
      if (state != 1)
        inerror("! Solved in wrong place");
      state++;
      expect(toks, 1);
      pz.id = allocsetval(pz, -1);
      uchar *p = pz.id.dat;
      for (int i = 0; i < (int)pz.setdefs.size(); i++) {
        int n = pz.setdefs[i].size;
        for (int j = 0; j < n; j++)
          p[j] = j;
        p += n;
        for (int j = 0; j < n; j++)
          p[j] = 0;
        p += n;
      }
      pz.solved = readposition(pz, 's', f, checksum, toks[0] == "StartState");
    } else if (toks[0] == "Move" || toks[0] == "MoveTransformation") {
      if (state != 2)
        inerror("! Move in wrong place");
      expect(toks, 2);
      moove m(pz, pz.id);
      m.name = toks[1];
      m.pos =
          readposition(pz, 'm', f, checksum, toks[0] == "MoveTransformation");
      m.cost = 1;
      m.twist = 1;
      addnewmove(pz, m, newnames);
    } else if (toks[0] == "MoveAlias") {
      if (state != 2)
        inerror("! MoveAlias in wrong place");
      expect(toks, 3);
      pz.moveseqs.push_back({toks[1], toks[2], 1});
    } else if (toks[0] == "MoveSequence") {
      if (state != 2)
        inerror("! MoveSequence in wrong place");
      if (toks.size() < 3)
        inerror("Too few tokens in MoveSequence definition");
      string seq;
      for (int i = 2; i < (int)toks.size(); i++) {
        if (i != 2)
          seq += " ";
        seq += toks[i];
      }
      pz.moveseqs.push_back({toks[1], seq, 0});
    } else {
      inerror("! unexpected first token on line ", toks[0]);
    }
  }
  if (pz.name.size() == 0)
    inerror("! puzzle must be given a name");
  if (pz.setdefs.size() == 0)
    inerror("! puzzle must have set definitions");
  if (pz.solved.dat == 0)
    inerror("! puzzle must have a solved position");
  if (pz.moves.size() == 0)
    inerror("! puzzle must have moves");
  lineno = 0;
  if (distinguishall) {
    pz.solved = pz.id;
  }
  pz.caninvert = pz.uniq && !pz.wildo;
  pz.doubleprobe = pz.uniq && !pz.wildo && pz.defaultstart();
  pz.checksum = checksum;
  curline.clear();
  if (setsmustexist.size()) {
    cerr << "The following sets were specified on the command line but "
            "don't exist in the puzzle definition:"
         << endl;
    for (auto &s : setsmustexist)
      cerr << "   " << s << endl;
    error("! sets specified on command line don't exist");
  }
  expandalgomoves(pz, newnames);
  return pz;
}
