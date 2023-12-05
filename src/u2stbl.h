typedef unsigned short UniCode;
typedef unsigned short SJisCode;

struct U2S {
	UniCode uni;
	SJisCode sjis;
};

extern struct U2S U2STable[];

extern SJisCode Uni2SJis( UniCode uni );
