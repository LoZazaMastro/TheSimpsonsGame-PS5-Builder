#include "simpsons_init.h"

DEFINE_REX_FUNC(sub_82CB9D60) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9896(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9896);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12200(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12200, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9D80) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// stfs f0,12228(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12228, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9D98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12224(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12224, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9DB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12248(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12248, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9DD8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12244(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12244, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9DF8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// bl 0x82745bf8
	ctx.lr = 0x82CB9E08;
	sub_82745BF8(ctx, base);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// lwz r10,0(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// addi r11,r11,12232
	ctx.r11.s64 = ctx.r11.s64 + 12232;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9E38) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9852(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9852);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// stfs f0,12280(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12280, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9E50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9852(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9852);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12276(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12276, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9E70) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9852(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9852);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12288(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12288, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9E90) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,-9852(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -9852);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,12284(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 12284, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9EB0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,768
	ctx.r3.s64 = 768;
	// bl 0x8269be40
	ctx.lr = 0x82CB9EC4;
	sub_8269BE40(ctx, base);
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,12368
	ctx.r11.s64 = ctx.r11.s64 + 12368;
	// stw r3,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r3.u32);
	// addi r3,r10,5936
	ctx.r3.s64 = ctx.r10.s64 + 5936;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// li r10,64
	ctx.r10.s64 = 64;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CB9EEC;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9F00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5904
	ctx.r3.s64 = ctx.r11.s64 + 5904;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CB9F10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5920
	ctx.r3.s64 = ctx.r11.s64 + 5920;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CB9F20) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5952
	ctx.r3.s64 = ctx.r11.s64 + 5952;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CB9F30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5960
	ctx.r3.s64 = ctx.r11.s64 + 5960;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CB9F40) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,920(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 920);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13660(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13660, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9F58) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,920(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 920);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13664(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13664, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9F78) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,920(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 920);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13652(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13652, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9F98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,920(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 920);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13656(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13656, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9FB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f0,-11484(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f13,916(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 916);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// stfs f0,-13668(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13668, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CB9FD8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5984
	ctx.r3.s64 = ctx.r11.s64 + 5984;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CB9FE8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,940(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 940);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13588(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13588, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA000) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,940(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 940);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13592(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13592, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA020) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,940(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 940);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13580(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13580, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA040) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,940(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 940);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13584(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13584, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA060) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,5992
	ctx.r3.s64 = ctx.r11.s64 + 5992;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA070) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r30,-24(r1)
	REX_STORE_U64(ctx.r1.u32 + -24, ctx.r30.u64);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lis r30,-32043
	ctx.r30.s64 = -2099970048;
	// addi r31,r11,964
	ctx.r31.s64 = ctx.r11.s64 + 964;
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lwz r3,29252(r30)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r30.u32 + 29252);
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// stw r31,-13560(r11)
	REX_STORE_U32(ctx.r11.u32 + -13560, ctx.r31.u32);
	// bne cr6,0x82cba0ac
	if (!ctx.cr6.eq) goto loc_82CBA0AC;
	// bl 0x8268e7f0
	ctx.lr = 0x82CBA0A8;
	sub_8268E7F0(ctx, base);
	// stw r3,29252(r30)
	REX_STORE_U32(ctx.r30.u32 + 29252, ctx.r3.u32);
loc_82CBA0AC:
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// lwz r11,8(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 8);
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82CBA0BC;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// bl 0x823297f0
	ctx.lr = 0x82CBA0C0;
	sub_823297F0(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// stw r3,44(r31)
	REX_STORE_U32(ctx.r31.u32 + 44, ctx.r3.u32);
	// addi r3,r11,6000
	ctx.r3.s64 = ctx.r11.s64 + 6000;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA0D0;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r30,-24(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -24);
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA0E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13516(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13516, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA100) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13520(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13520, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA120) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13508(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13508, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA140) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13512(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13512, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA160) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1036(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1036);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13212(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13212, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA178) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1036(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1036);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13224(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13224, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA198) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1036(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1036);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13200(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13200, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA1B8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1036(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1036);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13208(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13208, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA1D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1028(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1028);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13216(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13216, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA1F0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,2992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f13,1028(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1028);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// stfs f0,-13220(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13220, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA210) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,2992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lfs f13,-13216(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -13216);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// stfs f0,-13204(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13204, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA230) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lfs f0,-13216(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -13216);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-12060(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12060);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// fmuls f13,f0,f13
	ctx.f13.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// lfs f0,2992(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// stfs f0,-13192(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13192, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA260) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,5108(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lfs f13,-13216(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -13216);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// stfs f0,-13196(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13196, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA280) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1040(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1040);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-13184(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13184, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA298) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1040(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1040);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13188(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13188, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA2B8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1040(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1040);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13172(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13172, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA2D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1040(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1040);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-13176(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -13176, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA2F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32245
	ctx.r11.s64 = -2113208320;
	// lis r10,-32235
	ctx.r10.s64 = -2112552960;
	// lfs f0,28300(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 28300);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f13,1028(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1028);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fdivs f13,f0,f13
	ctx.f13.f64 = double(float(ctx.f0.f64 / ctx.f13.f64));
	// lfs f0,-5936(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -5936);
	ctx.f0.f64 = double(temp.f32);
	// addi r11,r11,-13180
	ctx.r11.s64 = ctx.r11.s64 + -13180;
	// fmuls f0,f13,f0
	ctx.f0.f64 = double(float(ctx.f13.f64 * ctx.f0.f64));
	// fctidz f0,f0
	ctx.f0.s64 = std::isnan(ctx.f0.f64) ? int64_t(0x8000000000000000ULL) : (ctx.f0.f64 > double(LLONG_MAX)) ? LLONG_MAX : simde_mm_cvttsd_si64(simde_mm_load_sd(&ctx.f0.f64));
	// stfiwx f0,0,r11
	REX_STORE_U32(ctx.r11.u32, ctx.f0.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA330) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6144
	ctx.r3.s64 = ctx.r11.s64 + 6144;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA340) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6152
	ctx.r3.s64 = ctx.r11.s64 + 6152;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA350) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6160
	ctx.r3.s64 = ctx.r11.s64 + 6160;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA360) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6168
	ctx.r3.s64 = ctx.r11.s64 + 6168;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA370) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// addi r3,r11,-11528
	ctx.r3.s64 = ctx.r11.s64 + -11528;
	// bl 0x82c1cdd8
	ctx.lr = 0x82CBA388;
	sub_82C1CDD8(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6176
	ctx.r3.s64 = ctx.r11.s64 + 6176;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA394;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA3A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1180(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1180);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-10232(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10232, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA3C0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1180(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1180);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10236(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10236, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA3E0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1180(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1180);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10224(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10224, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA400) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,1180(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 1180);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10228(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10228, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA420) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,3024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// stfs f0,-10108(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10108, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA438) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,3024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10112(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10112, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA458) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,3024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10100(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10100, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA478) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,3024(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3024);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-10104(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -10104, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA498) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// addi r8,r11,-10072
	ctx.r8.s64 = ctx.r11.s64 + -10072;
	// li r11,16383
	ctx.r11.s64 = 16383;
	// mr r10,r8
	ctx.r10.u64 = ctx.r8.u64;
loc_82CBA4A8:
	// addi r9,r10,36
	ctx.r9.s64 = ctx.r10.s64 + 36;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stw r9,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r9.u32);
	// mr r10,r9
	ctx.r10.u64 = ctx.r9.u64;
	// bne cr6,0x82cba4a8
	if (!ctx.cr6.eq) goto loc_82CBA4A8;
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// lis r10,8
	ctx.r10.s64 = 524288;
	// addi r3,r11,6264
	ctx.r3.s64 = ctx.r11.s64 + 6264;
	// ori r10,r10,65500
	ctx.r10.u64 = ctx.r10.u64 | 65500;
	// li r11,0
	ctx.r11.s64 = 0;
	// stwx r11,r8,r10
	REX_STORE_U32(ctx.r8.u32 + ctx.r10.u32, ctx.r11.u32);
	// lis r11,9
	ctx.r11.s64 = 589824;
	// lis r10,9
	ctx.r10.s64 = 589824;
	// ori r10,r10,4
	ctx.r10.u64 = ctx.r10.u64 | 4;
	// stwx r8,r8,r11
	REX_STORE_U32(ctx.r8.u32 + ctx.r11.u32, ctx.r8.u32);
	// li r11,16384
	ctx.r11.s64 = 16384;
	// stwx r11,r8,r10
	REX_STORE_U32(ctx.r8.u32 + ctx.r10.u32, ctx.r11.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA4F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6272
	ctx.r3.s64 = ctx.r11.s64 + 6272;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA508) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,-10040
	ctx.r3.s64 = ctx.r11.s64 + -10040;
	// bl 0x8274a8d0
	ctx.lr = 0x82CBA520;
	sub_8274A8D0(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6304
	ctx.r3.s64 = ctx.r11.s64 + 6304;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA52C;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA540) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,-10032
	ctx.r3.s64 = ctx.r11.s64 + -10032;
	// bl 0x82742b90
	ctx.lr = 0x82CBA558;
	sub_82742B90(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6320
	ctx.r3.s64 = ctx.r11.s64 + 6320;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA564;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA578) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,2052
	ctx.r3.s64 = 2052;
	// bl 0x8269be40
	ctx.lr = 0x82CBA590;
	sub_8269BE40(ctx, base);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r31,r11,8008
	ctx.r31.s64 = ctx.r11.s64 + 8008;
	// stw r3,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r3.u32);
	// lwz r11,4(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 4);
	// rlwinm r5,r11,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// bl 0x82a3c440
	ctx.lr = 0x82CBA5AC;
	sub_82A3C440(ctx, base);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274edd8
	ctx.lr = 0x82CBA5B4;
	sub_8274EDD8(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6336
	ctx.r3.s64 = ctx.r11.s64 + 6336;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA5C0;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA5D8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,2052
	ctx.r3.s64 = 2052;
	// bl 0x8269be40
	ctx.lr = 0x82CBA5EC;
	sub_8269BE40(ctx, base);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r11,r11,8032
	ctx.r11.s64 = ctx.r11.s64 + 8032;
	// stw r3,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r3.u32);
	// lwz r11,4(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// rlwinm r5,r11,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// bl 0x82a3c440
	ctx.lr = 0x82CBA608;
	sub_82A3C440(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6400
	ctx.r3.s64 = ctx.r11.s64 + 6400;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA614;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA628) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// addi r11,r11,-4352
	ctx.r11.s64 = ctx.r11.s64 + -4352;
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// li r10,0
	ctx.r10.s64 = 0;
	// lfs f0,-12072(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
loc_82CBA644:
	// stfs f0,0(r11)
	ctx.fpscr.disableFlushMode();
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// addi r9,r11,52
	ctx.r9.s64 = ctx.r11.s64 + 52;
	// stfs f0,4(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// sth r10,-4(r11)
	REX_STORE_U16(ctx.r11.u32 + -4, ctx.r10.u16);
	// li r8,10
	ctx.r8.s64 = 10;
	// sth r10,-2(r11)
	REX_STORE_U16(ctx.r11.u32 + -2, ctx.r10.u16);
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// stb r10,92(r11)
	REX_STORE_U8(ctx.r11.u32 + 92, ctx.r10.u8);
loc_82CBA664:
	// addi r8,r8,-1
	ctx.r8.s64 = ctx.r8.s64 + -1;
	// stw r10,-40(r9)
	REX_STORE_U32(ctx.r9.u32 + -40, ctx.r10.u32);
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// bne cr6,0x82cba664
	if (!ctx.cr6.eq) goto loc_82CBA664;
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// addi r11,r11,100
	ctx.r11.s64 = ctx.r11.s64 + 100;
	// cmpwi cr6,r7,0
	ctx.cr6.compare<int32_t>(ctx.r7.s32, 0, ctx.xer);
	// bge cr6,0x82cba644
	if (!ctx.cr6.lt) goto loc_82CBA644;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA690) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,32
	ctx.r3.s64 = 32;
	// bl 0x8269be40
	ctx.lr = 0x82CBA6A4;
	sub_8269BE40(ctx, base);
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,8060
	ctx.r11.s64 = ctx.r11.s64 + 8060;
	// stw r3,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r3.u32);
	// addi r3,r10,6464
	ctx.r3.s64 = ctx.r10.s64 + 6464;
	// li r10,8
	ctx.r10.s64 = 8;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r10.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA6CC;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA6E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6504
	ctx.r3.s64 = ctx.r11.s64 + 6504;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA6F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9092
	ctx.r11.s64 = ctx.r11.s64 + 9092;
	// lis r9,-32235
	ctx.r9.s64 = -2112552960;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,12220
	ctx.r9.s64 = ctx.r9.s64 + 12220;
	// addi r3,r8,6512
	ctx.r3.s64 = ctx.r8.s64 + 6512;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA720) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9268
	ctx.r11.s64 = ctx.r11.s64 + 9268;
	// lis r9,-32235
	ctx.r9.s64 = -2112552960;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,17876
	ctx.r9.s64 = ctx.r9.s64 + 17876;
	// addi r3,r8,6536
	ctx.r3.s64 = ctx.r8.s64 + 6536;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA750) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r4,32
	ctx.r4.s64 = 32;
	// addi r3,r11,-3288
	ctx.r3.s64 = ctx.r11.s64 + -3288;
	// bl 0x8276e9c8
	ctx.lr = 0x82CBA76C;
	sub_8276E9C8(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6560
	ctx.r3.s64 = ctx.r11.s64 + 6560;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBA778;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBA788) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,-3280
	ctx.r3.s64 = ctx.r11.s64 + -3280;
	// b 0x8276ed68
	sub_8276ED68(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA798) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9780
	ctx.r11.s64 = ctx.r11.s64 + 9780;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-31488
	ctx.r9.s64 = ctx.r9.s64 + -31488;
	// addi r3,r8,6576
	ctx.r3.s64 = ctx.r8.s64 + 6576;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA7C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9828
	ctx.r11.s64 = ctx.r11.s64 + 9828;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-31368
	ctx.r9.s64 = ctx.r9.s64 + -31368;
	// addi r3,r8,6600
	ctx.r3.s64 = ctx.r8.s64 + 6600;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA7F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9876
	ctx.r11.s64 = ctx.r11.s64 + 9876;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-31240
	ctx.r9.s64 = ctx.r9.s64 + -31240;
	// addi r3,r8,6624
	ctx.r3.s64 = ctx.r8.s64 + 6624;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA828) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9924
	ctx.r11.s64 = ctx.r11.s64 + 9924;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-31124
	ctx.r9.s64 = ctx.r9.s64 + -31124;
	// addi r3,r8,6648
	ctx.r3.s64 = ctx.r8.s64 + 6648;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA858) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,9972
	ctx.r11.s64 = ctx.r11.s64 + 9972;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-31008
	ctx.r9.s64 = ctx.r9.s64 + -31008;
	// addi r3,r8,6672
	ctx.r3.s64 = ctx.r8.s64 + 6672;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA888) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10020
	ctx.r11.s64 = ctx.r11.s64 + 10020;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30808
	ctx.r9.s64 = ctx.r9.s64 + -30808;
	// addi r3,r8,6696
	ctx.r3.s64 = ctx.r8.s64 + 6696;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA8B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10068
	ctx.r11.s64 = ctx.r11.s64 + 10068;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30560
	ctx.r9.s64 = ctx.r9.s64 + -30560;
	// addi r3,r8,6720
	ctx.r3.s64 = ctx.r8.s64 + 6720;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA8E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10116
	ctx.r11.s64 = ctx.r11.s64 + 10116;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30436
	ctx.r9.s64 = ctx.r9.s64 + -30436;
	// addi r3,r8,6744
	ctx.r3.s64 = ctx.r8.s64 + 6744;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA918) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10164
	ctx.r11.s64 = ctx.r11.s64 + 10164;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30312
	ctx.r9.s64 = ctx.r9.s64 + -30312;
	// addi r3,r8,6768
	ctx.r3.s64 = ctx.r8.s64 + 6768;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA948) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10212
	ctx.r11.s64 = ctx.r11.s64 + 10212;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30196
	ctx.r9.s64 = ctx.r9.s64 + -30196;
	// addi r3,r8,6792
	ctx.r3.s64 = ctx.r8.s64 + 6792;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA978) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10260
	ctx.r11.s64 = ctx.r11.s64 + 10260;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-30080
	ctx.r9.s64 = ctx.r9.s64 + -30080;
	// addi r3,r8,6816
	ctx.r3.s64 = ctx.r8.s64 + 6816;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA9A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10308
	ctx.r11.s64 = ctx.r11.s64 + 10308;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-29952
	ctx.r9.s64 = ctx.r9.s64 + -29952;
	// addi r3,r8,6840
	ctx.r3.s64 = ctx.r8.s64 + 6840;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBA9D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10356
	ctx.r11.s64 = ctx.r11.s64 + 10356;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-29836
	ctx.r9.s64 = ctx.r9.s64 + -29836;
	// addi r3,r8,6864
	ctx.r3.s64 = ctx.r8.s64 + 6864;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAA08) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10404
	ctx.r11.s64 = ctx.r11.s64 + 10404;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-29720
	ctx.r9.s64 = ctx.r9.s64 + -29720;
	// addi r3,r8,6888
	ctx.r3.s64 = ctx.r8.s64 + 6888;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAA38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r11,r11,-2680
	ctx.r11.s64 = ctx.r11.s64 + -2680;
	// addi r10,r11,-200
	ctx.r10.s64 = ctx.r11.s64 + -200;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAA58) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10456
	ctx.r11.s64 = ctx.r11.s64 + 10456;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-29604
	ctx.r9.s64 = ctx.r9.s64 + -29604;
	// addi r3,r8,6912
	ctx.r3.s64 = ctx.r8.s64 + 6912;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAA88) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,10496
	ctx.r11.s64 = ctx.r11.s64 + 10496;
	// lis r9,-32234
	ctx.r9.s64 = -2112487424;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-29544
	ctx.r9.s64 = ctx.r9.s64 + -29544;
	// addi r3,r8,6936
	ctx.r3.s64 = ctx.r8.s64 + 6936;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAAB8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6960
	ctx.r3.s64 = ctx.r11.s64 + 6960;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAAC8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,5
	ctx.r8.s64 = 5;
	// lis r11,-32234
	ctx.r11.s64 = -2112487424;
	// addi r4,r10,-26568
	ctx.r4.s64 = ctx.r10.s64 + -26568;
	// addi r11,r11,-26464
	ctx.r11.s64 = ctx.r11.s64 + -26464;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-1428
	ctx.r3.s64 = ctx.r10.s64 + -1428;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBAB18;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAB28) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-27160
	ctx.r5.s64 = ctx.r10.s64 + -27160;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32234
	ctx.r11.s64 = -2112487424;
	// addi r4,r10,-26292
	ctx.r4.s64 = ctx.r10.s64 + -26292;
	// addi r11,r11,-26332
	ctx.r11.s64 = ctx.r11.s64 + -26332;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-1392
	ctx.r3.s64 = ctx.r10.s64 + -1392;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,208
	ctx.r6.s64 = 208;
	// bl 0x82b647f8
	ctx.lr = 0x82CBAB7C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAB90) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-29532
	ctx.r5.s64 = ctx.r10.s64 + -29532;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32234
	ctx.r11.s64 = -2112487424;
	// addi r4,r10,-26172
	ctx.r4.s64 = ctx.r10.s64 + -26172;
	// addi r11,r11,-26232
	ctx.r11.s64 = ctx.r11.s64 + -26232;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-1356
	ctx.r3.s64 = ctx.r10.s64 + -1356;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,104
	ctx.r6.s64 = 104;
	// bl 0x82b647f8
	ctx.lr = 0x82CBABE4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBABF8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24492(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24492);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,6888(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6888, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAC10) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24492(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24492);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6876(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6876, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAC30) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24492(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24492);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6944(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6944, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAC50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24492(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24492);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6892(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6892, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAC70) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r5,1
	ctx.r5.s64 = 1;
	// addi r3,r11,6896
	ctx.r3.s64 = ctx.r11.s64 + 6896;
	// li r4,0
	ctx.r4.s64 = 0;
	// bl 0x82329660
	ctx.lr = 0x82CBAC90;
	sub_82329660(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6968
	ctx.r3.s64 = ctx.r11.s64 + 6968;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBAC9C;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBACB0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6984
	ctx.r3.s64 = ctx.r11.s64 + 6984;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBACC0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,6992
	ctx.r3.s64 = ctx.r11.s64 + 6992;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBACD0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24504(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24504);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,6972(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6972, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBACE8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24504(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24504);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6960(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6960, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAD08) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24504(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24504);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6980(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6980, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAD28) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,24504(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 24504);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6976(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6976, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAD48) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7000
	ctx.r3.s64 = ctx.r11.s64 + 7000;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAD58) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7008
	ctx.r3.s64 = ctx.r11.s64 + 7008;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAD68) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r3,r10,7016
	ctx.r3.s64 = ctx.r10.s64 + 7016;
	// lhz r9,25600(r11)
	ctx.r9.u64 = REX_LOAD_U16(ctx.r11.u32 + 25600);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// sth r9,25600(r11)
	REX_STORE_U16(ctx.r11.u32 + 25600, ctx.r9.u16);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAD88) {
	REX_FUNC_PROLOGUE();
	// lis r9,-32049
	ctx.r9.s64 = -2100363264;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r9,r9,25600
	ctx.r9.s64 = ctx.r9.s64 + 25600;
	// addi r11,r11,9592
	ctx.r11.s64 = ctx.r11.s64 + 9592;
	// li r10,89
	ctx.r10.s64 = 89;
	// lhz r8,0(r9)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r9.u32 + 0);
loc_82CBADA0:
	// clrlwi r8,r8,16
	ctx.r8.u64 = ctx.r8.u32 & 0xFFFF;
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// stw r9,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// clrlwi r8,r8,16
	ctx.r8.u64 = ctx.r8.u32 & 0xFFFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// clrlwi r8,r8,16
	ctx.r8.u64 = ctx.r8.u32 & 0xFFFF;
	// bne cr6,0x82cbada0
	if (!ctx.cr6.eq) goto loc_82CBADA0;
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// sth r8,0(r9)
	REX_STORE_U16(ctx.r9.u32 + 0, ctx.r8.u16);
	// addi r3,r11,7032
	ctx.r3.s64 = ctx.r11.s64 + 7032;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBADE0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7120
	ctx.r3.s64 = ctx.r11.s64 + 7120;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBADF0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r3,r10,7136
	ctx.r3.s64 = ctx.r10.s64 + 7136;
	// lhz r9,25600(r11)
	ctx.r9.u64 = REX_LOAD_U16(ctx.r11.u32 + 25600);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// sth r9,25600(r11)
	REX_STORE_U16(ctx.r11.u32 + 25600, ctx.r9.u16);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAE10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7152
	ctx.r3.s64 = ctx.r11.s64 + 7152;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAE20) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7160
	ctx.r3.s64 = ctx.r11.s64 + 7160;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAE30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7168
	ctx.r3.s64 = ctx.r11.s64 + 7168;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAE40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7176
	ctx.r3.s64 = ctx.r11.s64 + 7176;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAE50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,28436(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28436, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAE68) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28432(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28432, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAE88) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28444(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28444, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAEA8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28440(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28440, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAEC8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7184
	ctx.r3.s64 = ctx.r11.s64 + 7184;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAED8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7192
	ctx.r3.s64 = ctx.r11.s64 + 7192;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAEE8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7200
	ctx.r3.s64 = ctx.r11.s64 + 7200;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAEF8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7208
	ctx.r3.s64 = ctx.r11.s64 + 7208;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAF08) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7216
	ctx.r3.s64 = ctx.r11.s64 + 7216;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBAF18) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32580(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32580);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,28492(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28492, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAF30) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32580(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32580);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28488(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28488, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAF50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32580(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32580);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28500(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28500, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAF70) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// lfs f0,32580(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 32580);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,28496(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 28496, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAF90) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31524(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31524);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29052(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29052, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAFA8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31524(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31524);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29048(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29048, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAFC8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31524(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31524);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29060(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29060, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBAFE8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31524(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31524);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29056(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29056, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB008) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31516(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31516);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29068(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29068, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB020) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31516(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31516);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29064(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29064, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB040) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31516(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31516);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29076(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29076, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB060) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31516(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31516);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29072(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29072, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB080) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31508(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31508);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29100(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29100, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB098) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31508(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31508);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29088(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29088, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB0B8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31508(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31508);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29116(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29116, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB0D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31508(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31508);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29104(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29104, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB0F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7224
	ctx.r3.s64 = ctx.r11.s64 + 7224;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB108) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7232
	ctx.r3.s64 = ctx.r11.s64 + 7232;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB118) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7240
	ctx.r3.s64 = ctx.r11.s64 + 7240;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB128) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31484(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31484);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29176(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29176, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB140) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31484(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31484);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29172(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29172, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB160) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31484(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31484);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29184(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29184, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB180) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31484(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31484);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29180(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29180, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB1A0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29208(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29208, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB1B8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29204(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29204, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB1D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29216(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29216, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB1F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29212(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29212, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB218) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31448(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31448);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29224(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29224, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB230) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31448(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31448);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29220(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29220, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB250) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31448(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31448);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29248(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29248, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB270) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31448(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31448);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29236(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29236, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB290) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7248
	ctx.r3.s64 = ctx.r11.s64 + 7248;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB2A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7256
	ctx.r3.s64 = ctx.r11.s64 + 7256;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB2B0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31408(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31408);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29264(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29264, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB2C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31408(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31408);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29260(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29260, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB2E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31408(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31408);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29272(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29272, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB308) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31408(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31408);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29268(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29268, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB328) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31400(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31400);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29280(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29280, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB340) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31400(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31400);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29276(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29276, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB360) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31400(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31400);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29288(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29288, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB380) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31400(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31400);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29284(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29284, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB3A0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31392(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31392);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29296(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29296, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB3B8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31392(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31392);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29292(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29292, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB3D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31392(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31392);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29304(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29304, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB3F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31392(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31392);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29300(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29300, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB418) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31364(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31364);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29384(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29384, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB430) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31364(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31364);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29376(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29376, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB450) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31364(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31364);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29392(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29392, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB470) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31364(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31364);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29388(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29388, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB490) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31360(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31360);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-12060(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12060);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,29396(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29396, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB4B0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-31360(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -31360);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,29380(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 29380, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB4C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,-31348
	ctx.r11.s64 = ctx.r11.s64 + -31348;
	// lis r9,-32232
	ctx.r9.s64 = -2112356352;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,-100
	ctx.r9.s64 = ctx.r9.s64 + -100;
	// addi r3,r8,7264
	ctx.r3.s64 = ctx.r8.s64 + 7264;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB4F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30060(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30060, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB510) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30048(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30048, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB530) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30076(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30076, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB550) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30064(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30064, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB570) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7288
	ctx.r3.s64 = ctx.r11.s64 + 7288;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB580) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7296
	ctx.r3.s64 = ctx.r11.s64 + 7296;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB590) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30096(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30096, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB5A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30092(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30092, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB5C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30104(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30104, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB5E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30100(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30100, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB608) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30976(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30976);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30124(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30124, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB620) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30976(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30976);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30120(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30120, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB640) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30976(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30976);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30132(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30132, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB660) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30976(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30976);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30128(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30128, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB680) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7304
	ctx.r3.s64 = ctx.r11.s64 + 7304;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB690) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30152(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30152, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB6A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30148(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30148, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB6C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30160(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30160, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB6E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30888(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30888);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30156(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30156, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB708) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7328
	ctx.r3.s64 = ctx.r11.s64 + 7328;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB718) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30744(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30744);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30356(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30356, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB730) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30744(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30744);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30352(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30352, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB750) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30744(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30744);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30364(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30364, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB770) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30744(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30744);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30360(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30360, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB790) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30736(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30736);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30372(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30372, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB7A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30736(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30736);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30368(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30368, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB7C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30736(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30736);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30380(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30380, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB7E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30736(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30736);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30376(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30376, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB808) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30728(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30728);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30388(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30388, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB820) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30728(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30728);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30384(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30384, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB840) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30728(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30728);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30396(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30396, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB860) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30728(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30728);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30392(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30392, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB880) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7360
	ctx.r3.s64 = ctx.r11.s64 + 7360;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB890) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30688(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30688);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30480(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30480, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB8A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30688(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30688);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30412(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30412, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB8C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30688(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30688);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30552(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30552, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB8E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30688(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30688);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30508(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30508, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBB908) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7368
	ctx.r3.s64 = ctx.r11.s64 + 7368;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB918) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7376
	ctx.r3.s64 = ctx.r11.s64 + 7376;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB928) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7384
	ctx.r3.s64 = ctx.r11.s64 + 7384;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB938) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7392
	ctx.r3.s64 = ctx.r11.s64 + 7392;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB948) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7400
	ctx.r3.s64 = ctx.r11.s64 + 7400;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB958) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7408
	ctx.r3.s64 = ctx.r11.s64 + 7408;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB968) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7416
	ctx.r3.s64 = ctx.r11.s64 + 7416;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB978) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7424
	ctx.r3.s64 = ctx.r11.s64 + 7424;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB988) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7432
	ctx.r3.s64 = ctx.r11.s64 + 7432;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB998) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7440
	ctx.r3.s64 = ctx.r11.s64 + 7440;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7448
	ctx.r3.s64 = ctx.r11.s64 + 7448;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7456
	ctx.r3.s64 = ctx.r11.s64 + 7456;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7464
	ctx.r3.s64 = ctx.r11.s64 + 7464;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7472
	ctx.r3.s64 = ctx.r11.s64 + 7472;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7480
	ctx.r3.s64 = ctx.r11.s64 + 7480;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBB9F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7488
	ctx.r3.s64 = ctx.r11.s64 + 7488;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBA08) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7496
	ctx.r3.s64 = ctx.r11.s64 + 7496;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBA18) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7504
	ctx.r3.s64 = ctx.r11.s64 + 7504;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBA28) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7512
	ctx.r3.s64 = ctx.r11.s64 + 7512;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBA38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7520
	ctx.r3.s64 = ctx.r11.s64 + 7520;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBA48) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30588(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30588);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,30988(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30988, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBA60) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30588(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30588);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,30936(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 30936, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBA80) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30588(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30588);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31092(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31092, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBAA0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30588(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30588);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31032(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31032, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBAC0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7528
	ctx.r3.s64 = ctx.r11.s64 + 7528;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBAD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7536
	ctx.r3.s64 = ctx.r11.s64 + 7536;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBAE0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7544
	ctx.r3.s64 = ctx.r11.s64 + 7544;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBAF0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7552
	ctx.r3.s64 = ctx.r11.s64 + 7552;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7560
	ctx.r3.s64 = ctx.r11.s64 + 7560;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7568
	ctx.r3.s64 = ctx.r11.s64 + 7568;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB20) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7576
	ctx.r3.s64 = ctx.r11.s64 + 7576;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7584
	ctx.r3.s64 = ctx.r11.s64 + 7584;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7592
	ctx.r3.s64 = ctx.r11.s64 + 7592;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB50) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7600
	ctx.r3.s64 = ctx.r11.s64 + 7600;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB60) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7608
	ctx.r3.s64 = ctx.r11.s64 + 7608;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB70) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7616
	ctx.r3.s64 = ctx.r11.s64 + 7616;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB80) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7624
	ctx.r3.s64 = ctx.r11.s64 + 7624;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBB90) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7632
	ctx.r3.s64 = ctx.r11.s64 + 7632;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7640
	ctx.r3.s64 = ctx.r11.s64 + 7640;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBB0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7648
	ctx.r3.s64 = ctx.r11.s64 + 7648;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBC0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7656
	ctx.r3.s64 = ctx.r11.s64 + 7656;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7664
	ctx.r3.s64 = ctx.r11.s64 + 7664;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBE0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7672
	ctx.r3.s64 = ctx.r11.s64 + 7672;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBBF0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7680
	ctx.r3.s64 = ctx.r11.s64 + 7680;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBC00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7688
	ctx.r3.s64 = ctx.r11.s64 + 7688;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBC10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7696
	ctx.r3.s64 = ctx.r11.s64 + 7696;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBC20) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31132(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31132, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBC38) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31128(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31128, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBC58) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31140(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31140, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBC78) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30572(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30572);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31136(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31136, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBC98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30556(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30556);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31172(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31172, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBCB0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30556(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30556);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31144(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31144, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBCD0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30556(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30556);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31188(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31188, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBCF0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30556(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30556);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31184(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31184, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBD10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7704
	ctx.r3.s64 = ctx.r11.s64 + 7704;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBD20) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7712
	ctx.r3.s64 = ctx.r11.s64 + 7712;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBD30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7720
	ctx.r3.s64 = ctx.r11.s64 + 7720;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBD40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7728
	ctx.r3.s64 = ctx.r11.s64 + 7728;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBD50) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r5,1
	ctx.r5.s64 = 1;
	// addi r3,r11,31208
	ctx.r3.s64 = ctx.r11.s64 + 31208;
	// li r4,0
	ctx.r4.s64 = 0;
	// bl 0x82329660
	ctx.lr = 0x82CBBD70;
	sub_82329660(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7736
	ctx.r3.s64 = ctx.r11.s64 + 7736;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBBD7C;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBD90) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7752
	ctx.r3.s64 = ctx.r11.s64 + 7752;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBDA0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31280(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31280, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBDB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31268(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31268, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBDD8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31288(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31288, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBDF8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31284(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31284, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBE18) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7760
	ctx.r3.s64 = ctx.r11.s64 + 7760;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBE28) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7768
	ctx.r3.s64 = ctx.r11.s64 + 7768;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBE38) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32232
	ctx.r11.s64 = -2112356352;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r6,r11,19800
	ctx.r6.s64 = ctx.r11.s64 + 19800;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r31,r11,31300
	ctx.r31.s64 = ctx.r11.s64 + 31300;
	// li r5,32
	ctx.r5.s64 = 32;
	// li r4,12
	ctx.r4.s64 = 12;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274aa98
	ctx.lr = 0x82CBBE70;
	sub_8274AA98(ctx, base);
	// lis r11,-32232
	ctx.r11.s64 = -2112356352;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,19788
	ctx.r11.s64 = ctx.r11.s64 + 19788;
	// addi r3,r10,7776
	ctx.r3.s64 = ctx.r10.s64 + 7776;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBBE88;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBEA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7800
	ctx.r3.s64 = ctx.r11.s64 + 7800;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBEB0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7808
	ctx.r3.s64 = ctx.r11.s64 + 7808;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBEC0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30176(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30176);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31380(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31380, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBED8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30176(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30176);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31376(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31376, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBEF8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30176(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30176);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31412(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31412, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBF18) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30176(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30176);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31392(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31392, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBF38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7816
	ctx.r3.s64 = ctx.r11.s64 + 7816;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF48) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7824
	ctx.r3.s64 = ctx.r11.s64 + 7824;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF58) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7832
	ctx.r3.s64 = ctx.r11.s64 + 7832;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF68) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7840
	ctx.r3.s64 = ctx.r11.s64 + 7840;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF78) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7848
	ctx.r3.s64 = ctx.r11.s64 + 7848;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF88) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7856
	ctx.r3.s64 = ctx.r11.s64 + 7856;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBBF98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30100(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30100);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31476(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31476, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBFB0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30100(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30100);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31440(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31440, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBFD0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30100(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30100);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31524(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31524, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBBFF0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-30100(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -30100);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31488(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31488, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC010) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7864
	ctx.r3.s64 = ctx.r11.s64 + 7864;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC020) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7872
	ctx.r3.s64 = ctx.r11.s64 + 7872;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC030) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7880
	ctx.r3.s64 = ctx.r11.s64 + 7880;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC040) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7888
	ctx.r3.s64 = ctx.r11.s64 + 7888;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC050) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7896
	ctx.r3.s64 = ctx.r11.s64 + 7896;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC060) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7904
	ctx.r3.s64 = ctx.r11.s64 + 7904;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC070) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7912
	ctx.r3.s64 = ctx.r11.s64 + 7912;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC080) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7920
	ctx.r3.s64 = ctx.r11.s64 + 7920;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC090) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7928
	ctx.r3.s64 = ctx.r11.s64 + 7928;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC0A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7936
	ctx.r3.s64 = ctx.r11.s64 + 7936;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC0B0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7944
	ctx.r3.s64 = ctx.r11.s64 + 7944;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC0C0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29864(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29864);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31548(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31548, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC0D8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29864(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29864);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31544(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31544, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC0F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29864(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29864);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31556(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31556, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC118) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29864(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29864);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31552(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31552, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC138) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7952
	ctx.r3.s64 = ctx.r11.s64 + 7952;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC148) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7960
	ctx.r3.s64 = ctx.r11.s64 + 7960;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC158) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7968
	ctx.r3.s64 = ctx.r11.s64 + 7968;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC168) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7976
	ctx.r3.s64 = ctx.r11.s64 + 7976;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC178) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7984
	ctx.r3.s64 = ctx.r11.s64 + 7984;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC188) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,7992
	ctx.r3.s64 = ctx.r11.s64 + 7992;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC198) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8000
	ctx.r3.s64 = ctx.r11.s64 + 8000;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8008
	ctx.r3.s64 = ctx.r11.s64 + 8008;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8016
	ctx.r3.s64 = ctx.r11.s64 + 8016;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8024
	ctx.r3.s64 = ctx.r11.s64 + 8024;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8032
	ctx.r3.s64 = ctx.r11.s64 + 8032;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8040
	ctx.r3.s64 = ctx.r11.s64 + 8040;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC1F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8048
	ctx.r3.s64 = ctx.r11.s64 + 8048;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC208) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8056
	ctx.r3.s64 = ctx.r11.s64 + 8056;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC218) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8064
	ctx.r3.s64 = ctx.r11.s64 + 8064;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC228) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8072
	ctx.r3.s64 = ctx.r11.s64 + 8072;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC238) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8080
	ctx.r3.s64 = ctx.r11.s64 + 8080;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC248) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8088
	ctx.r3.s64 = ctx.r11.s64 + 8088;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC258) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8096
	ctx.r3.s64 = ctx.r11.s64 + 8096;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC268) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8104
	ctx.r3.s64 = ctx.r11.s64 + 8104;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC278) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8112
	ctx.r3.s64 = ctx.r11.s64 + 8112;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC288) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8120
	ctx.r3.s64 = ctx.r11.s64 + 8120;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC298) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8128
	ctx.r3.s64 = ctx.r11.s64 + 8128;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8136
	ctx.r3.s64 = ctx.r11.s64 + 8136;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8144
	ctx.r3.s64 = ctx.r11.s64 + 8144;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8152
	ctx.r3.s64 = ctx.r11.s64 + 8152;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8160
	ctx.r3.s64 = ctx.r11.s64 + 8160;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8168
	ctx.r3.s64 = ctx.r11.s64 + 8168;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC2F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8176
	ctx.r3.s64 = ctx.r11.s64 + 8176;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC308) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8184
	ctx.r3.s64 = ctx.r11.s64 + 8184;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC318) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8192
	ctx.r3.s64 = ctx.r11.s64 + 8192;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC328) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8200
	ctx.r3.s64 = ctx.r11.s64 + 8200;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC338) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8208
	ctx.r3.s64 = ctx.r11.s64 + 8208;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC348) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8216
	ctx.r3.s64 = ctx.r11.s64 + 8216;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC358) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8224
	ctx.r3.s64 = ctx.r11.s64 + 8224;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC368) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8232
	ctx.r3.s64 = ctx.r11.s64 + 8232;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC378) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8240
	ctx.r3.s64 = ctx.r11.s64 + 8240;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC388) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8248
	ctx.r3.s64 = ctx.r11.s64 + 8248;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC398) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8256
	ctx.r3.s64 = ctx.r11.s64 + 8256;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8264
	ctx.r3.s64 = ctx.r11.s64 + 8264;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8272
	ctx.r3.s64 = ctx.r11.s64 + 8272;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8280
	ctx.r3.s64 = ctx.r11.s64 + 8280;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8288
	ctx.r3.s64 = ctx.r11.s64 + 8288;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8296
	ctx.r3.s64 = ctx.r11.s64 + 8296;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC3F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8304
	ctx.r3.s64 = ctx.r11.s64 + 8304;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC408) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8312
	ctx.r3.s64 = ctx.r11.s64 + 8312;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC418) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8320
	ctx.r3.s64 = ctx.r11.s64 + 8320;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC428) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8328
	ctx.r3.s64 = ctx.r11.s64 + 8328;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC438) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8336
	ctx.r3.s64 = ctx.r11.s64 + 8336;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC448) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29260(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29260);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,31996(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31996, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC460) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29260(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29260);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,31992(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 31992, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC480) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29260(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29260);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32004(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32004, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC4A0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29260(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29260);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32000(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32000, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC4C0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8344
	ctx.r3.s64 = ctx.r11.s64 + 8344;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC4D0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8352
	ctx.r3.s64 = ctx.r11.s64 + 8352;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC4E0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29224(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29224);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,32068(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32068, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC4F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29224(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29224);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32064(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32064, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC518) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29224(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29224);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32076(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32076, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC538) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29224(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29224);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32072(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32072, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC558) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,13312
	ctx.r3.s64 = 13312;
	// bl 0x8269be40
	ctx.lr = 0x82CBC56C;
	sub_8269BE40(ctx, base);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,32052
	ctx.r11.s64 = ctx.r11.s64 + 32052;
	// stw r3,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r3.u32);
	// addi r3,r10,8376
	ctx.r3.s64 = ctx.r10.s64 + 8376;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// li r10,256
	ctx.r10.s64 = 256;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBC594;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC5A8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,768
	ctx.r3.s64 = 768;
	// bl 0x8269be40
	ctx.lr = 0x82CBC5BC;
	sub_8269BE40(ctx, base);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,32080
	ctx.r11.s64 = ctx.r11.s64 + 32080;
	// stw r3,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r3.u32);
	// addi r3,r10,8360
	ctx.r3.s64 = ctx.r10.s64 + 8360;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// li r10,32
	ctx.r10.s64 = 32;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBC5E4;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC5F8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29168(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29168);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,32096(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32096, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC610) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29168(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29168);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32092(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32092, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC630) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29168(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29168);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32104(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32104, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC650) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29168(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29168);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32100(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32100, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC670) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8392
	ctx.r3.s64 = ctx.r11.s64 + 8392;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC680) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8400
	ctx.r3.s64 = ctx.r11.s64 + 8400;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC690) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29088(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29088);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f0,32164(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32164, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC6A8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29088(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29088);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32144(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32144, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC6C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29088(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29088);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32172(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32172, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC6E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-29088(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29088);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,32168(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 32168, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC708) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8408
	ctx.r3.s64 = ctx.r11.s64 + 8408;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC718) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8416
	ctx.r3.s64 = ctx.r11.s64 + 8416;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC728) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8424
	ctx.r3.s64 = ctx.r11.s64 + 8424;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC738) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8432
	ctx.r3.s64 = ctx.r11.s64 + 8432;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC748) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// addi r3,r11,-20820
	ctx.r3.s64 = ctx.r11.s64 + -20820;
	// bl 0x827451c0
	ctx.lr = 0x82CBC760;
	sub_827451C0(ctx, base);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stw r3,32292(r11)
	REX_STORE_U32(ctx.r11.u32 + 32292, ctx.r3.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC778) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8440
	ctx.r3.s64 = ctx.r11.s64 + 8440;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC788) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r6,r11,-18588
	ctx.r6.s64 = ctx.r11.s64 + -18588;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r7,1
	ctx.r7.s64 = 1;
	// addi r31,r11,32324
	ctx.r31.s64 = ctx.r11.s64 + 32324;
	// li r5,8
	ctx.r5.s64 = 8;
	// li r4,116
	ctx.r4.s64 = 116;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274aa98
	ctx.lr = 0x82CBC7C0;
	sub_8274AA98(ctx, base);
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,-18768
	ctx.r11.s64 = ctx.r11.s64 + -18768;
	// addi r3,r10,8448
	ctx.r3.s64 = ctx.r10.s64 + 8448;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBC7D8;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC7F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,32380
	ctx.r3.s64 = ctx.r11.s64 + 32380;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC800) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,32388
	ctx.r3.s64 = ctx.r11.s64 + 32388;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC810) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f0,-12072(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// stfs f0,-16(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -16, temp.u32);
	// stfs f0,-8(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -8, temp.u32);
	// stfs f0,-4(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -4, temp.u32);
	// lfs f13,2992(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// stfs f13,-12(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -12, temp.u32);
	// addi r10,r1,-16
	ctx.r10.s64 = ctx.r1.s64 + -16;
	// addi r11,r11,32400
	ctx.r11.s64 = ctx.r11.s64 + 32400;
	// lvx128 v0,r0,r10
	ea = (ctx.r10.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC848) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8480
	ctx.r3.s64 = ctx.r11.s64 + 8480;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC858) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8488
	ctx.r3.s64 = ctx.r11.s64 + 8488;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC868) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r31,r11,-28528
	ctx.r31.s64 = ctx.r11.s64 + -28528;
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// li r6,28
	ctx.r6.s64 = 28;
	// addi r4,r11,-31416
	ctx.r4.s64 = ctx.r11.s64 + -31416;
	// li r5,3584
	ctx.r5.s64 = 3584;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270ca78
	ctx.lr = 0x82CBC89C;
	sub_8270CA78(ctx, base);
	// li r11,1
	ctx.r11.s64 = 1;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// stw r11,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r11.u32);
	// bl 0x8270cc08
	ctx.lr = 0x82CBC8B0;
	sub_8270CC08(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8496
	ctx.r3.s64 = ctx.r11.s64 + 8496;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBC8BC;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC8D0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r31,r11,-28488
	ctx.r31.s64 = ctx.r11.s64 + -28488;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r6,12
	ctx.r6.s64 = 12;
	// addi r4,r11,32584
	ctx.r4.s64 = ctx.r11.s64 + 32584;
	// li r5,1536
	ctx.r5.s64 = 1536;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270ca78
	ctx.lr = 0x82CBC904;
	sub_8270CA78(ctx, base);
	// li r11,1
	ctx.r11.s64 = 1;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// stw r11,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r11.u32);
	// bl 0x8270cc08
	ctx.lr = 0x82CBC918;
	sub_8270CC08(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8560
	ctx.r3.s64 = ctx.r11.s64 + 8560;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBC924;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC938) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8624
	ctx.r3.s64 = ctx.r11.s64 + 8624;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC948) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8632
	ctx.r3.s64 = ctx.r11.s64 + 8632;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC958) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-28344(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -28344);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// stfs f0,-27800(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -27800, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC970) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-28344(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -28344);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-27804(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -27804, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC990) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-28344(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -28344);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-27792(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -27792, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC9B0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-28344(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -28344);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-27796(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -27796, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBC9D0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8640
	ctx.r3.s64 = ctx.r11.s64 + 8640;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC9E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8648
	ctx.r3.s64 = ctx.r11.s64 + 8648;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBC9F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8656
	ctx.r3.s64 = ctx.r11.s64 + 8656;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8664
	ctx.r3.s64 = ctx.r11.s64 + 8664;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8672
	ctx.r3.s64 = ctx.r11.s64 + 8672;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA20) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8680
	ctx.r3.s64 = ctx.r11.s64 + 8680;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8688
	ctx.r3.s64 = ctx.r11.s64 + 8688;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8696
	ctx.r3.s64 = ctx.r11.s64 + 8696;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA50) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8704
	ctx.r3.s64 = ctx.r11.s64 + 8704;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA60) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8712
	ctx.r3.s64 = ctx.r11.s64 + 8712;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA70) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8720
	ctx.r3.s64 = ctx.r11.s64 + 8720;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA80) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8728
	ctx.r3.s64 = ctx.r11.s64 + 8728;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCA90) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8736
	ctx.r3.s64 = ctx.r11.s64 + 8736;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCAA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8744
	ctx.r3.s64 = ctx.r11.s64 + 8744;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCAB0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8752
	ctx.r3.s64 = ctx.r11.s64 + 8752;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCAC0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8760
	ctx.r3.s64 = ctx.r11.s64 + 8760;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCAD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8768
	ctx.r3.s64 = ctx.r11.s64 + 8768;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCAE0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f0,-12072(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// stfs f0,-16(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -16, temp.u32);
	// stfs f0,-12(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -12, temp.u32);
	// stfs f0,-4(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -4, temp.u32);
	// lfs f13,2992(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// stfs f13,-8(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -8, temp.u32);
	// addi r10,r1,-16
	ctx.r10.s64 = ctx.r1.s64 + -16;
	// addi r11,r11,-27536
	ctx.r11.s64 = ctx.r11.s64 + -27536;
	// lvx128 v0,r0,r10
	ea = (ctx.r10.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCB18) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8776
	ctx.r3.s64 = ctx.r11.s64 + 8776;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB28) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8784
	ctx.r3.s64 = ctx.r11.s64 + 8784;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8792
	ctx.r3.s64 = ctx.r11.s64 + 8792;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB48) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8800
	ctx.r3.s64 = ctx.r11.s64 + 8800;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB58) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8808
	ctx.r3.s64 = ctx.r11.s64 + 8808;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB68) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8816
	ctx.r3.s64 = ctx.r11.s64 + 8816;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCB78) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-27428(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -27428);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32245
	ctx.r11.s64 = -2113208320;
	// lfs f13,13156(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 13156);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27084(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27084, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCB98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-27432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -27432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// lfs f13,4744(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4744);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27080(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27080, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCBB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-27432(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -27432);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11364(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11364);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27076(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27076, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCBD8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r5,1
	ctx.r5.s64 = 65536;
	// addi r31,r11,-27380
	ctx.r31.s64 = ctx.r11.s64 + -27380;
	// lis r11,-32031
	ctx.r11.s64 = -2099183616;
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r4,r11,-27400
	ctx.r4.s64 = ctx.r11.s64 + -27400;
	// li r6,68
	ctx.r6.s64 = 68;
	// ori r5,r5,36464
	ctx.r5.u64 = ctx.r5.u64 | 36464;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270ca78
	ctx.lr = 0x82CBCC10;
	sub_8270CA78(ctx, base);
	// li r11,1
	ctx.r11.s64 = 1;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// stw r11,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r11.u32);
	// bl 0x8270cc08
	ctx.lr = 0x82CBCC24;
	sub_8270CC08(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8824
	ctx.r3.s64 = ctx.r11.s64 + 8824;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBCC30;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCC48) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r31,r11,-27340
	ctx.r31.s64 = ctx.r11.s64 + -27340;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// li r6,12
	ctx.r6.s64 = 12;
	// addi r4,r11,9064
	ctx.r4.s64 = ctx.r11.s64 + 9064;
	// li r5,18000
	ctx.r5.s64 = 18000;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270ca78
	ctx.lr = 0x82CBCC7C;
	sub_8270CA78(ctx, base);
	// li r11,1
	ctx.r11.s64 = 1;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// stw r11,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r11.u32);
	// bl 0x8270cc08
	ctx.lr = 0x82CBCC90;
	sub_8270CC08(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8888
	ctx.r3.s64 = ctx.r11.s64 + 8888;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBCC9C;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCCB0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8952
	ctx.r3.s64 = ctx.r11.s64 + 8952;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCCC0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// lfd f1,5920(r11)
	ctx.fpscr.disableFlushMode();
	ctx.f1.u64 = REX_LOAD_U64(ctx.r11.u32 + 5920);
	// bl 0x82a3dab0
	ctx.lr = 0x82CBCCD8;
	sub_82A3DAB0(ctx, base);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// frsp f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// stfs f0,27176(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27176, temp.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCCF8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8960
	ctx.r3.s64 = ctx.r11.s64 + 8960;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCD08) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8968
	ctx.r3.s64 = ctx.r11.s64 + 8968;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCD18) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8976
	ctx.r3.s64 = ctx.r11.s64 + 8976;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCD28) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,8984
	ctx.r3.s64 = ctx.r11.s64 + 8984;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCD38) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r3,512
	ctx.r3.s64 = 512;
	// bl 0x8269be40
	ctx.lr = 0x82CBCD4C;
	sub_8269BE40(ctx, base);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,27276
	ctx.r11.s64 = ctx.r11.s64 + 27276;
	// stw r3,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r3.u32);
	// addi r3,r10,8992
	ctx.r3.s64 = ctx.r10.s64 + 8992;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// li r10,128
	ctx.r10.s64 = 128;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBCD74;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCD88) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9008
	ctx.r3.s64 = ctx.r11.s64 + 9008;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCD98) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9016
	ctx.r3.s64 = ctx.r11.s64 + 9016;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCDA8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9024
	ctx.r3.s64 = ctx.r11.s64 + 9024;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCDB8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9032
	ctx.r3.s64 = ctx.r11.s64 + 9032;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCDC8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9040
	ctx.r3.s64 = ctx.r11.s64 + 9040;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCDD8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-26564(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -26564);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27372(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27372, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCDF0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-26564(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -26564);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27368(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27368, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCE10) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-26564(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -26564);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27380(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27380, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCE30) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-26564(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -26564);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27376(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27376, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCE50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-25056(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -25056);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27400(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27400, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCE68) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-25056(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -25056);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27396(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27396, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCE88) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-25056(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -25056);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27408(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27408, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCEA8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-25056(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -25056);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27404(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27404, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCEC8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27416(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27416, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCEE0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27412(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27412, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCF00) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27424(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27424, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCF20) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24984(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24984);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27420(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27420, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCF40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9048
	ctx.r3.s64 = ctx.r11.s64 + 9048;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBCF50) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24972(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24972);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27440(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27440, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCF68) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24972(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24972);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27436(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27436, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCF88) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24972(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24972);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27448(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27448, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCFA8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24972(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24972);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27444(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27444, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCFC8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24932(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24932);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27456(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27456, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBCFE0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24932(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24932);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27452(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27452, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD000) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24932(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24932);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27464(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27464, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD020) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24932(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24932);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27460(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27460, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD040) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,-24608
	ctx.r11.s64 = ctx.r11.s64 + -24608;
	// lis r9,-32231
	ctx.r9.s64 = -2112290816;
	// lis r8,-32052
	ctx.r8.s64 = -2100559872;
	// addi r9,r9,24708
	ctx.r9.s64 = ctx.r9.s64 + 24708;
	// addi r3,r8,9056
	ctx.r3.s64 = ctx.r8.s64 + 9056;
	// stw r10,28(r11)
	REX_STORE_U32(ctx.r11.u32 + 28, ctx.r10.u32);
	// stw r10,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r10.u32);
	// stw r10,36(r11)
	REX_STORE_U32(ctx.r11.u32 + 36, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD070) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// std r30,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r30.u64);
	// std r31,-8(r1)
	REX_STORE_U64(ctx.r1.u32 + -8, ctx.r31.u64);
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// lis r8,-32256
	ctx.r8.s64 = -2113929216;
	// lis r10,-32226
	ctx.r10.s64 = -2111963136;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// lis r7,-32226
	ctx.r7.s64 = -2111963136;
	// lfs f13,-11616(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -11616);
	ctx.f13.f64 = double(temp.f32);
	// lis r9,-32231
	ctx.r9.s64 = -2112290816;
	// lfs f12,2992(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 2992);
	ctx.f12.f64 = double(temp.f32);
	// lis r8,-32235
	ctx.r8.s64 = -2112552960;
	// stfs f13,-76(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -76, temp.u32);
	// addi r11,r11,27472
	ctx.r11.s64 = ctx.r11.s64 + 27472;
	// lfs f0,-12028(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -12028);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r1,-80
	ctx.r10.s64 = ctx.r1.s64 + -80;
	// addi r4,r11,32
	ctx.r4.s64 = ctx.r11.s64 + 32;
	// stfs f0,-80(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -80, temp.u32);
	// lfs f13,2868(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 2868);
	ctx.f13.f64 = double(temp.f32);
	// lis r9,-32256
	ctx.r9.s64 = -2113929216;
	// lfs f11,-5432(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -5432);
	ctx.f11.f64 = double(temp.f32);
	// lis r8,-32226
	ctx.r8.s64 = -2111963136;
	// stfs f13,-72(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -72, temp.u32);
	// addi r3,r11,48
	ctx.r3.s64 = ctx.r11.s64 + 48;
	// stfs f11,-60(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -60, temp.u32);
	// mr r6,r11
	ctx.r6.u64 = ctx.r11.u64;
	// addi r5,r11,16
	ctx.r5.s64 = ctx.r11.s64 + 16;
	// stfs f0,-52(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -52, temp.u32);
	// lfs f13,4024(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4024);
	ctx.f13.f64 = double(temp.f32);
	// addi r9,r1,-64
	ctx.r9.s64 = ctx.r1.s64 + -64;
	// lfs f11,-12060(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -12060);
	ctx.f11.f64 = double(temp.f32);
	// mr r11,r4
	ctx.r11.u64 = ctx.r4.u64;
	// stfs f0,-48(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -48, temp.u32);
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// lfs f0,-12072(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// addi r8,r1,-48
	ctx.r8.s64 = ctx.r1.s64 + -48;
	// stfs f13,-68(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -68, temp.u32);
	// ld r3,0(r10)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// mr r7,r6
	ctx.r7.u64 = ctx.r6.u64;
	// ld r10,8(r10)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r10.u32 + 8);
	// addi r6,r1,-32
	ctx.r6.s64 = ctx.r1.s64 + -32;
	// stfs f12,-64(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -64, temp.u32);
	// stfs f11,-56(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -56, temp.u32);
	// ld r31,0(r9)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// ld r9,8(r9)
	ctx.r9.u64 = REX_LOAD_U64(ctx.r9.u32 + 8);
	// stfs f0,-44(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -44, temp.u32);
	// stfs f13,-40(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -40, temp.u32);
	// stfs f0,-36(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -36, temp.u32);
	// ld r30,0(r8)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// std r3,0(r7)
	REX_STORE_U64(ctx.r7.u32 + 0, ctx.r3.u64);
	// stfs f0,-32(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -32, temp.u32);
	// ld r8,8(r8)
	ctx.r8.u64 = REX_LOAD_U64(ctx.r8.u32 + 8);
	// stfs f12,-28(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -28, temp.u32);
	// stfs f0,-24(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -24, temp.u32);
	// std r10,8(r7)
	REX_STORE_U64(ctx.r7.u32 + 8, ctx.r10.u64);
	// stfs f0,-20(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -20, temp.u32);
	// ld r3,0(r6)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r6.u32 + 0);
	// ld r10,8(r6)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r6.u32 + 8);
	// std r31,0(r5)
	REX_STORE_U64(ctx.r5.u32 + 0, ctx.r31.u64);
	// std r9,8(r5)
	REX_STORE_U64(ctx.r5.u32 + 8, ctx.r9.u64);
	// std r30,0(r11)
	REX_STORE_U64(ctx.r11.u32 + 0, ctx.r30.u64);
	// std r8,8(r11)
	REX_STORE_U64(ctx.r11.u32 + 8, ctx.r8.u64);
	// std r3,0(r4)
	REX_STORE_U64(ctx.r4.u32 + 0, ctx.r3.u64);
	// std r10,8(r4)
	REX_STORE_U64(ctx.r4.u32 + 8, ctx.r10.u64);
	// ld r30,-16(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// ld r31,-8(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD178) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// std r30,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r30.u64);
	// std r31,-8(r1)
	REX_STORE_U64(ctx.r1.u32 + -8, ctx.r31.u64);
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// lis r10,-32226
	ctx.r10.s64 = -2111963136;
	// lis r8,-32231
	ctx.r8.s64 = -2112290816;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r6,r1,-32
	ctx.r6.s64 = ctx.r1.s64 + -32;
	// lfs f13,-12064(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -12064);
	ctx.f13.f64 = double(temp.f32);
	// lis r9,-32256
	ctx.r9.s64 = -2113929216;
	// lfs f0,-12016(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -12016);
	ctx.f0.f64 = double(temp.f32);
	// addi r11,r11,27664
	ctx.r11.s64 = ctx.r11.s64 + 27664;
	// stfs f0,-80(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -80, temp.u32);
	// addi r10,r1,-80
	ctx.r10.s64 = ctx.r1.s64 + -80;
	// lfs f0,25332(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 25332);
	ctx.f0.f64 = double(temp.f32);
	// lis r8,-32226
	ctx.r8.s64 = -2111963136;
	// addi r4,r11,32
	ctx.r4.s64 = ctx.r11.s64 + 32;
	// stfs f0,-60(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -60, temp.u32);
	// lfs f12,9856(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 9856);
	ctx.f12.f64 = double(temp.f32);
	// lis r9,-32256
	ctx.r9.s64 = -2113929216;
	// addi r3,r11,48
	ctx.r3.s64 = ctx.r11.s64 + 48;
	// stfs f13,-76(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -76, temp.u32);
	// mr r7,r11
	ctx.r7.u64 = ctx.r11.u64;
	// stfs f12,-72(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -72, temp.u32);
	// addi r5,r11,16
	ctx.r5.s64 = ctx.r11.s64 + 16;
	// lfs f0,-12072(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// mr r11,r4
	ctx.r11.u64 = ctx.r4.u64;
	// stfs f13,-64(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -64, temp.u32);
	// lfs f11,2992(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 2992);
	ctx.f11.f64 = double(temp.f32);
	// addi r9,r1,-64
	ctx.r9.s64 = ctx.r1.s64 + -64;
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// stfs f11,-68(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -68, temp.u32);
	// addi r8,r1,-48
	ctx.r8.s64 = ctx.r1.s64 + -48;
	// ld r3,0(r10)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// ld r10,8(r10)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r10.u32 + 8);
	// stfs f13,-56(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -56, temp.u32);
	// stfs f0,-52(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -52, temp.u32);
	// ld r31,0(r9)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// stfs f12,-48(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -48, temp.u32);
	// ld r9,8(r9)
	ctx.r9.u64 = REX_LOAD_U64(ctx.r9.u32 + 8);
	// stfs f13,-44(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -44, temp.u32);
	// stfs f0,-40(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -40, temp.u32);
	// std r3,0(r7)
	REX_STORE_U64(ctx.r7.u32 + 0, ctx.r3.u64);
	// stfs f0,-36(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -36, temp.u32);
	// ld r30,0(r8)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// ld r8,8(r8)
	ctx.r8.u64 = REX_LOAD_U64(ctx.r8.u32 + 8);
	// stfs f11,-32(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -32, temp.u32);
	// stfs f0,-28(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -28, temp.u32);
	// std r10,8(r7)
	REX_STORE_U64(ctx.r7.u32 + 8, ctx.r10.u64);
	// stfs f0,-24(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -24, temp.u32);
	// std r31,0(r5)
	REX_STORE_U64(ctx.r5.u32 + 0, ctx.r31.u64);
	// stfs f0,-20(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -20, temp.u32);
	// ld r3,0(r6)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r6.u32 + 0);
	// ld r10,8(r6)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r6.u32 + 8);
	// std r9,8(r5)
	REX_STORE_U64(ctx.r5.u32 + 8, ctx.r9.u64);
	// std r30,0(r11)
	REX_STORE_U64(ctx.r11.u32 + 0, ctx.r30.u64);
	// std r8,8(r11)
	REX_STORE_U64(ctx.r11.u32 + 8, ctx.r8.u64);
	// std r3,0(r4)
	REX_STORE_U64(ctx.r4.u32 + 0, ctx.r3.u64);
	// std r10,8(r4)
	REX_STORE_U64(ctx.r4.u32 + 8, ctx.r10.u64);
	// ld r30,-16(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// ld r31,-8(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD270) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// std r30,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r30.u64);
	// std r31,-8(r1)
	REX_STORE_U64(ctx.r1.u32 + -8, ctx.r31.u64);
	// lis r10,-32231
	ctx.r10.s64 = -2112290816;
	// lis r9,-32256
	ctx.r9.s64 = -2113929216;
	// lis r8,-32226
	ctx.r8.s64 = -2111963136;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// lis r5,-32231
	ctx.r5.s64 = -2112290816;
	// lfs f0,25336(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 25336);
	ctx.f0.f64 = double(temp.f32);
	// addi r11,r11,27536
	ctx.r11.s64 = ctx.r11.s64 + 27536;
	// stfs f0,-80(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -80, temp.u32);
	// addi r10,r1,-80
	ctx.r10.s64 = ctx.r1.s64 + -80;
	// lfs f0,4024(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4024);
	ctx.f0.f64 = double(temp.f32);
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// lfs f13,-12016(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -12016);
	ctx.f13.f64 = double(temp.f32);
	// lis r8,-32226
	ctx.r8.s64 = -2111963136;
	// addi r4,r11,16
	ctx.r4.s64 = ctx.r11.s64 + 16;
	// stfs f0,-76(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -76, temp.u32);
	// stfs f0,-64(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -64, temp.u32);
	// addi r3,r11,32
	ctx.r3.s64 = ctx.r11.s64 + 32;
	// stfs f0,-56(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -56, temp.u32);
	// mr r7,r11
	ctx.r7.u64 = ctx.r11.u64;
	// lfs f11,-12028(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -12028);
	ctx.f11.f64 = double(temp.f32);
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// stfs f0,-40(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -40, temp.u32);
	// addi r6,r1,-32
	ctx.r6.s64 = ctx.r1.s64 + -32;
	// lfs f0,3948(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 3948);
	ctx.f0.f64 = double(temp.f32);
	// mr r5,r4
	ctx.r5.u64 = ctx.r4.u64;
	// stfs f13,-60(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -60, temp.u32);
	// addi r4,r11,48
	ctx.r4.s64 = ctx.r11.s64 + 48;
	// lfs f13,-12072(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -12072);
	ctx.f13.f64 = double(temp.f32);
	// mr r11,r3
	ctx.r11.u64 = ctx.r3.u64;
	// lfs f12,-11808(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -11808);
	ctx.f12.f64 = double(temp.f32);
	// addi r9,r1,-64
	ctx.r9.s64 = ctx.r1.s64 + -64;
	// stfs f11,-72(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -72, temp.u32);
	// addi r8,r1,-48
	ctx.r8.s64 = ctx.r1.s64 + -48;
	// stfs f12,-68(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -68, temp.u32);
	// ld r3,0(r10)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// ld r10,8(r10)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r10.u32 + 8);
	// stfs f13,-52(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -52, temp.u32);
	// stfs f11,-48(r1)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r1.u32 + -48, temp.u32);
	// ld r31,0(r9)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// stfs f13,-44(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -44, temp.u32);
	// ld r9,8(r9)
	ctx.r9.u64 = REX_LOAD_U64(ctx.r9.u32 + 8);
	// stfs f13,-36(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -36, temp.u32);
	// ld r30,0(r8)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// stfs f12,-32(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -32, temp.u32);
	// std r3,0(r7)
	REX_STORE_U64(ctx.r7.u32 + 0, ctx.r3.u64);
	// stfs f0,-28(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -28, temp.u32);
	// ld r8,8(r8)
	ctx.r8.u64 = REX_LOAD_U64(ctx.r8.u32 + 8);
	// stfs f12,-24(r1)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r1.u32 + -24, temp.u32);
	// stfs f13,-20(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -20, temp.u32);
	// std r10,8(r7)
	REX_STORE_U64(ctx.r7.u32 + 8, ctx.r10.u64);
	// ld r3,0(r6)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r6.u32 + 0);
	// ld r10,8(r6)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r6.u32 + 8);
	// std r31,0(r5)
	REX_STORE_U64(ctx.r5.u32 + 0, ctx.r31.u64);
	// std r9,8(r5)
	REX_STORE_U64(ctx.r5.u32 + 8, ctx.r9.u64);
	// std r30,0(r11)
	REX_STORE_U64(ctx.r11.u32 + 0, ctx.r30.u64);
	// std r8,8(r11)
	REX_STORE_U64(ctx.r11.u32 + 8, ctx.r8.u64);
	// std r3,0(r4)
	REX_STORE_U64(ctx.r4.u32 + 0, ctx.r3.u64);
	// std r10,8(r4)
	REX_STORE_U64(ctx.r4.u32 + 8, ctx.r10.u64);
	// ld r30,-16(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// ld r31,-8(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD370) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// std r30,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r30.u64);
	// std r31,-8(r1)
	REX_STORE_U64(ctx.r1.u32 + -8, ctx.r31.u64);
	// lis r10,-32256
	ctx.r10.s64 = -2113929216;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// addi r11,r11,27600
	ctx.r11.s64 = ctx.r11.s64 + 27600;
	// addi r8,r1,-48
	ctx.r8.s64 = ctx.r1.s64 + -48;
	// lfs f13,2992(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 2992);
	ctx.f13.f64 = double(temp.f32);
	// addi r10,r1,-80
	ctx.r10.s64 = ctx.r1.s64 + -80;
	// addi r4,r11,32
	ctx.r4.s64 = ctx.r11.s64 + 32;
	// stfs f13,-80(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -80, temp.u32);
	// lfs f0,-12072(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// addi r3,r11,48
	ctx.r3.s64 = ctx.r11.s64 + 48;
	// mr r7,r11
	ctx.r7.u64 = ctx.r11.u64;
	// stfs f0,-76(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -76, temp.u32);
	// addi r5,r11,16
	ctx.r5.s64 = ctx.r11.s64 + 16;
	// stfs f0,-72(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -72, temp.u32);
	// addi r9,r1,-64
	ctx.r9.s64 = ctx.r1.s64 + -64;
	// stfs f0,-68(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -68, temp.u32);
	// mr r11,r4
	ctx.r11.u64 = ctx.r4.u64;
	// stfs f0,-64(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -64, temp.u32);
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// ld r3,0(r10)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// ld r10,8(r10)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r10.u32 + 8);
	// addi r6,r1,-32
	ctx.r6.s64 = ctx.r1.s64 + -32;
	// stfs f13,-60(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -60, temp.u32);
	// stfs f0,-56(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -56, temp.u32);
	// stfs f0,-52(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -52, temp.u32);
	// ld r31,0(r9)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// ld r9,8(r9)
	ctx.r9.u64 = REX_LOAD_U64(ctx.r9.u32 + 8);
	// stfs f0,-48(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -48, temp.u32);
	// stfs f0,-44(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -44, temp.u32);
	// std r3,0(r7)
	REX_STORE_U64(ctx.r7.u32 + 0, ctx.r3.u64);
	// stfs f13,-40(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -40, temp.u32);
	// std r10,8(r7)
	REX_STORE_U64(ctx.r7.u32 + 8, ctx.r10.u64);
	// stfs f0,-36(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -36, temp.u32);
	// ld r30,0(r8)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// ld r8,8(r8)
	ctx.r8.u64 = REX_LOAD_U64(ctx.r8.u32 + 8);
	// stfs f0,-32(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -32, temp.u32);
	// stfs f0,-28(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -28, temp.u32);
	// std r31,0(r5)
	REX_STORE_U64(ctx.r5.u32 + 0, ctx.r31.u64);
	// stfs f0,-24(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -24, temp.u32);
	// std r9,8(r5)
	REX_STORE_U64(ctx.r5.u32 + 8, ctx.r9.u64);
	// stfs f13,-20(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -20, temp.u32);
	// ld r3,0(r6)
	ctx.r3.u64 = REX_LOAD_U64(ctx.r6.u32 + 0);
	// ld r10,8(r6)
	ctx.r10.u64 = REX_LOAD_U64(ctx.r6.u32 + 8);
	// std r30,0(r11)
	REX_STORE_U64(ctx.r11.u32 + 0, ctx.r30.u64);
	// std r8,8(r11)
	REX_STORE_U64(ctx.r11.u32 + 8, ctx.r8.u64);
	// std r3,0(r4)
	REX_STORE_U64(ctx.r4.u32 + 0, ctx.r3.u64);
	// std r10,8(r4)
	REX_STORE_U64(ctx.r4.u32 + 8, ctx.r10.u64);
	// ld r30,-16(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// ld r31,-8(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD448) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r6,r11,25876
	ctx.r6.s64 = ctx.r11.s64 + 25876;
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// li r7,1
	ctx.r7.s64 = 1;
	// addi r31,r11,27736
	ctx.r31.s64 = ctx.r11.s64 + 27736;
	// li r5,16
	ctx.r5.s64 = 16;
	// li r4,80
	ctx.r4.s64 = 80;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274aa98
	ctx.lr = 0x82CBD480;
	sub_8274AA98(ctx, base);
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// lis r10,-32052
	ctx.r10.s64 = -2100559872;
	// addi r11,r11,25860
	ctx.r11.s64 = ctx.r11.s64 + 25860;
	// addi r3,r10,9080
	ctx.r3.s64 = ctx.r10.s64 + 9080;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// bl 0x82a3cc28
	ctx.lr = 0x82CBD498;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD4B0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24500(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24500);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stfs f0,27796(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27796, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD4C8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24500(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24500);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27792(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27792, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD4E8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24500(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24500);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27804(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27804, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD508) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,-24500(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -24500);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,27800(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 27800, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD528) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27820
	ctx.r3.s64 = ctx.r11.s64 + 27820;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD538) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27828
	ctx.r3.s64 = ctx.r11.s64 + 27828;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD548) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27836
	ctx.r3.s64 = ctx.r11.s64 + 27836;
	// b 0x8231e4f8
	sub_8231E4F8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD558) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27868
	ctx.r3.s64 = ctx.r11.s64 + 27868;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD568) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27852
	ctx.r3.s64 = ctx.r11.s64 + 27852;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD578) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27860
	ctx.r3.s64 = ctx.r11.s64 + 27860;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD588) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27844
	ctx.r3.s64 = ctx.r11.s64 + 27844;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD598) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27884
	ctx.r3.s64 = ctx.r11.s64 + 27884;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27876
	ctx.r3.s64 = ctx.r11.s64 + 27876;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27900
	ctx.r3.s64 = ctx.r11.s64 + 27900;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27892
	ctx.r3.s64 = ctx.r11.s64 + 27892;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27940
	ctx.r3.s64 = ctx.r11.s64 + 27940;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27916
	ctx.r3.s64 = ctx.r11.s64 + 27916;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD5F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27924
	ctx.r3.s64 = ctx.r11.s64 + 27924;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD608) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27908
	ctx.r3.s64 = ctx.r11.s64 + 27908;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD618) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27932
	ctx.r3.s64 = ctx.r11.s64 + 27932;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD628) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27996
	ctx.r3.s64 = ctx.r11.s64 + 27996;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD638) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27956
	ctx.r3.s64 = ctx.r11.s64 + 27956;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD648) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28044
	ctx.r3.s64 = ctx.r11.s64 + 28044;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD658) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28020
	ctx.r3.s64 = ctx.r11.s64 + 28020;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD668) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28028
	ctx.r3.s64 = ctx.r11.s64 + 28028;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD678) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27948
	ctx.r3.s64 = ctx.r11.s64 + 27948;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD688) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28036
	ctx.r3.s64 = ctx.r11.s64 + 28036;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD698) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27988
	ctx.r3.s64 = ctx.r11.s64 + 27988;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27980
	ctx.r3.s64 = ctx.r11.s64 + 27980;
	// b 0x8231e5d0
	sub_8231E5D0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28004
	ctx.r3.s64 = ctx.r11.s64 + 28004;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27972
	ctx.r3.s64 = ctx.r11.s64 + 27972;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27964
	ctx.r3.s64 = ctx.r11.s64 + 27964;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6E8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28052
	ctx.r3.s64 = ctx.r11.s64 + 28052;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD6F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,28012
	ctx.r3.s64 = ctx.r11.s64 + 28012;
	// b 0x8231e6a8
	sub_8231E6A8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBD708) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32091
	ctx.r11.s64 = -2103115776;
	// addi r3,r11,17400
	ctx.r3.s64 = ctx.r11.s64 + 17400;
	// bl 0x82a3cc28
	ctx.lr = 0x82CBD720;
	sub_82A3CC28(ctx, base);
	// lis r11,-32091
	ctx.r11.s64 = -2103115776;
	// addi r3,r11,17208
	ctx.r3.s64 = ctx.r11.s64 + 17208;
	// bl 0x824333a0
	ctx.lr = 0x82CBD72C;
	sub_824333A0(ctx, base);
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// stw r3,28808(r11)
	REX_STORE_U32(ctx.r11.u32 + 28808, ctx.r3.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD748) {
	REX_FUNC_PROLOGUE();
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,-17408
	ctx.r11.s64 = ctx.r11.s64 + -17408;
	// lwz r9,-26200(r10)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r10.u32 + -26200);
	// stw r11,-26200(r10)
	REX_STORE_U32(ctx.r10.u32 + -26200, ctx.r11.u32);
	// stw r9,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD768) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,3
	ctx.r8.s64 = 3;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26852
	ctx.r4.s64 = ctx.r10.s64 + -26852;
	// addi r11,r11,-25504
	ctx.r11.s64 = ctx.r11.s64 + -25504;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30524
	ctx.r3.s64 = ctx.r10.s64 + -30524;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD7B8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD7C8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26880
	ctx.r4.s64 = ctx.r10.s64 + -26880;
	// addi r11,r11,-25444
	ctx.r11.s64 = ctx.r11.s64 + -25444;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30488
	ctx.r3.s64 = ctx.r10.s64 + -30488;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD818;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD828) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,3
	ctx.r8.s64 = 3;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26916
	ctx.r4.s64 = ctx.r10.s64 + -26916;
	// addi r11,r11,-25400
	ctx.r11.s64 = ctx.r11.s64 + -25400;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30452
	ctx.r3.s64 = ctx.r10.s64 + -30452;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD878;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD888) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27256
	ctx.r4.s64 = ctx.r10.s64 + -27256;
	// addi r11,r11,-25340
	ctx.r11.s64 = ctx.r11.s64 + -25340;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30416
	ctx.r3.s64 = ctx.r10.s64 + -30416;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD8D8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD8E8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,9
	ctx.r8.s64 = 9;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26940
	ctx.r4.s64 = ctx.r10.s64 + -26940;
	// addi r11,r11,-25168
	ctx.r11.s64 = ctx.r11.s64 + -25168;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30380
	ctx.r3.s64 = ctx.r10.s64 + -30380;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD938;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD948) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,11
	ctx.r8.s64 = 11;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26960
	ctx.r4.s64 = ctx.r10.s64 + -26960;
	// addi r11,r11,-24800
	ctx.r11.s64 = ctx.r11.s64 + -24800;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30344
	ctx.r3.s64 = ctx.r10.s64 + -30344;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,64
	ctx.r6.s64 = 64;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBD998;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD9A8) {
	REX_FUNC_PROLOGUE();
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,-16888
	ctx.r11.s64 = ctx.r11.s64 + -16888;
	// lwz r9,-26200(r10)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r10.u32 + -26200);
	// stw r11,-26200(r10)
	REX_STORE_U32(ctx.r10.u32 + -26200, ctx.r11.u32);
	// stw r9,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD9C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lwz r11,-24036(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -24036);
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cntlzw r10,r11
	ctx.r10.u64 = ctx.r11.u32 == 0 ? 32 : __builtin_clz(ctx.r11.u32);
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// rlwinm r10,r10,27,31,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 27) & 0x1;
	// addi r11,r11,-16876
	ctx.r11.s64 = ctx.r11.s64 + -16876;
	// stb r10,1(r11)
	REX_STORE_U8(ctx.r11.u32 + 1, ctx.r10.u8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBD9F0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-608(r1)
	ea = -608 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x82aadc00
	ctx.lr = 0x82CBDA08;
	sub_82AADC00(ctx, base);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r8,-32085
	ctx.r8.s64 = -2102722560;
	// addi r10,r11,-20940
	ctx.r10.s64 = ctx.r11.s64 + -20940;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r9,-32085
	ctx.r9.s64 = -2102722560;
	// addi r11,r11,-30300
	ctx.r11.s64 = ctx.r11.s64 + -30300;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r8,-7496
	ctx.r10.s64 = ctx.r8.s64 + -7496;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r9,-11736
	ctx.r10.s64 = ctx.r9.s64 + -11736;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// stw r10,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r10.u32);
	// addi r1,r1,608
	ctx.r1.s64 = ctx.r1.s64 + 608;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDA50) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-608(r1)
	ea = -608 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x82aadc00
	ctx.lr = 0x82CBDA68;
	sub_82AADC00(ctx, base);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r7,-32085
	ctx.r7.s64 = -2102722560;
	// addi r10,r11,-20900
	ctx.r10.s64 = ctx.r11.s64 + -20900;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r8,-32085
	ctx.r8.s64 = -2102722560;
	// addi r9,r11,-20924
	ctx.r9.s64 = ctx.r11.s64 + -20924;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r11,r11,-30284
	ctx.r11.s64 = ctx.r11.s64 + -30284;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r7,-3712
	ctx.r10.s64 = ctx.r7.s64 + -3712;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r8,-3640
	ctx.r10.s64 = ctx.r8.s64 + -3640;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// stw r9,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r9.u32);
	// addi r1,r1,608
	ctx.r1.s64 = ctx.r1.s64 + 608;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDAB0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-416(r1)
	ea = -416 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x82ab20c8
	ctx.lr = 0x82CBDAC8;
	sub_82AB20C8(ctx, base);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r8,-32085
	ctx.r8.s64 = -2102722560;
	// addi r10,r11,-20648
	ctx.r10.s64 = ctx.r11.s64 + -20648;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r9,-32085
	ctx.r9.s64 = -2102722560;
	// addi r11,r11,-30268
	ctx.r11.s64 = ctx.r11.s64 + -30268;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r8,5432
	ctx.r10.s64 = ctx.r8.s64 + 5432;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r9,3872
	ctx.r10.s64 = ctx.r9.s64 + 3872;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// stw r10,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r10.u32);
	// addi r1,r1,416
	ctx.r1.s64 = ctx.r1.s64 + 416;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDB10) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,6
	ctx.r7.s64 = 6;
	// addi r9,r10,-20408
	ctx.r9.s64 = ctx.r10.s64 + -20408;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// addi r11,r11,-20392
	ctx.r11.s64 = ctx.r11.s64 + -20392;
	// addi r4,r10,-20272
	ctx.r4.s64 = ctx.r10.s64 + -20272;
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-30252
	ctx.r3.s64 = ctx.r10.s64 + -30252;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// li r6,88
	ctx.r6.s64 = 88;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBDB68;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDB78) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-29388
	ctx.r5.s64 = ctx.r10.s64 + -29388;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-20900
	ctx.r4.s64 = ctx.r10.s64 + -20900;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-30216
	ctx.r3.s64 = ctx.r10.s64 + -30216;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,512
	ctx.r6.s64 = 512;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDBC4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDBD8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-29300
	ctx.r5.s64 = ctx.r10.s64 + -29300;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-20224
	ctx.r4.s64 = ctx.r10.s64 + -20224;
	// addi r11,r11,-20244
	ctx.r11.s64 = ctx.r11.s64 + -20244;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30180
	ctx.r3.s64 = ctx.r10.s64 + -30180;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,304
	ctx.r6.s64 = 304;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDC2C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDC40) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r9,r10,-20080
	ctx.r9.s64 = ctx.r10.s64 + -20080;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r5,r10,-30072
	ctx.r5.s64 = ctx.r10.s64 + -30072;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r4,r10,-20068
	ctx.r4.s64 = ctx.r10.s64 + -20068;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r6,8
	ctx.r6.s64 = 8;
	// addi r3,r10,-30144
	ctx.r3.s64 = ctx.r10.s64 + -30144;
	// li r10,1
	ctx.r10.s64 = 1;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDC90;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDCA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-15312
	ctx.r10.s64 = ctx.r10.s64 + -15312;
	// lwz r11,-15320(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -15320);
	// stw r11,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDCB8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,8
	ctx.r7.s64 = 8;
	// addi r9,r10,-19728
	ctx.r9.s64 = ctx.r10.s64 + -19728;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// addi r11,r11,-15312
	ctx.r11.s64 = ctx.r11.s64 + -15312;
	// addi r4,r10,-19576
	ctx.r4.s64 = ctx.r10.s64 + -19576;
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-30108
	ctx.r3.s64 = ctx.r10.s64 + -30108;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// li r6,288
	ctx.r6.s64 = 288;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBDD10;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDD20) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-19564
	ctx.r4.s64 = ctx.r10.s64 + -19564;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-30072
	ctx.r3.s64 = ctx.r10.s64 + -30072;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,8
	ctx.r6.s64 = 8;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDD6C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDD80) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26664
	ctx.r4.s64 = ctx.r10.s64 + -26664;
	// addi r11,r11,-19360
	ctx.r11.s64 = ctx.r11.s64 + -19360;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30000
	ctx.r3.s64 = ctx.r10.s64 + -30000;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDDD0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDDE0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,7
	ctx.r8.s64 = 7;
	// addi r5,r10,-30144
	ctx.r5.s64 = ctx.r10.s64 + -30144;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-19180
	ctx.r4.s64 = ctx.r10.s64 + -19180;
	// addi r11,r11,-19320
	ctx.r11.s64 = ctx.r11.s64 + -19320;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-30036
	ctx.r3.s64 = ctx.r10.s64 + -30036;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,112
	ctx.r6.s64 = 112;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDE34;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDE48) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26680
	ctx.r4.s64 = ctx.r10.s64 + -26680;
	// addi r11,r11,-19120
	ctx.r11.s64 = ctx.r11.s64 + -19120;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29928
	ctx.r3.s64 = ctx.r10.s64 + -29928;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDE98;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDEA8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,3
	ctx.r8.s64 = 3;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26692
	ctx.r4.s64 = ctx.r10.s64 + -26692;
	// addi r11,r11,-19100
	ctx.r11.s64 = ctx.r11.s64 + -19100;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29964
	ctx.r3.s64 = ctx.r10.s64 + -29964;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDEF8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDF08) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-18952
	ctx.r4.s64 = ctx.r10.s64 + -18952;
	// addi r11,r11,-19012
	ctx.r11.s64 = ctx.r11.s64 + -19012;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29892
	ctx.r3.s64 = ctx.r10.s64 + -29892;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,64
	ctx.r6.s64 = 64;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDF58;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDF68) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28256
	ctx.r5.s64 = ctx.r10.s64 + -28256;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-18916
	ctx.r4.s64 = ctx.r10.s64 + -18916;
	// addi r11,r11,-18972
	ctx.r11.s64 = ctx.r11.s64 + -18972;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29856
	ctx.r3.s64 = ctx.r10.s64 + -29856;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,80
	ctx.r6.s64 = 80;
	// bl 0x82b647f8
	ctx.lr = 0x82CBDFBC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBDFD0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r9,r11,-18744
	ctx.r9.s64 = ctx.r11.s64 + -18744;
	// li r7,6
	ctx.r7.s64 = 6;
	// addi r8,r9,32
	ctx.r8.s64 = ctx.r9.s64 + 32;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// li r6,208
	ctx.r6.s64 = 208;
	// addi r4,r11,-18592
	ctx.r4.s64 = ctx.r11.s64 + -18592;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r3,r11,-29784
	ctx.r3.s64 = ctx.r11.s64 + -29784;
	// li r8,0
	ctx.r8.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE024;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE038) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28256
	ctx.r5.s64 = ctx.r10.s64 + -28256;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-18556
	ctx.r4.s64 = ctx.r10.s64 + -18556;
	// addi r11,r11,-18732
	ctx.r11.s64 = ctx.r11.s64 + -18732;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29820
	ctx.r3.s64 = ctx.r10.s64 + -29820;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,224
	ctx.r6.s64 = 224;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE08C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE0A0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r9,r11,-18516
	ctx.r9.s64 = ctx.r11.s64 + -18516;
	// li r7,3
	ctx.r7.s64 = 3;
	// addi r8,r9,12
	ctx.r8.s64 = ctx.r9.s64 + 12;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// li r6,160
	ctx.r6.s64 = 160;
	// addi r4,r11,-18424
	ctx.r4.s64 = ctx.r11.s64 + -18424;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r3,r11,-29748
	ctx.r3.s64 = ctx.r11.s64 + -29748;
	// li r8,0
	ctx.r8.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE0F4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE108) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28256
	ctx.r5.s64 = ctx.r10.s64 + -28256;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-18396
	ctx.r4.s64 = ctx.r10.s64 + -18396;
	// addi r11,r11,-18444
	ctx.r11.s64 = ctx.r11.s64 + -18444;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29712
	ctx.r3.s64 = ctx.r10.s64 + -29712;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,176
	ctx.r6.s64 = 176;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE15C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE170) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-15128
	ctx.r10.s64 = ctx.r10.s64 + -15128;
	// lwz r11,-15144(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -15144);
	// stw r11,88(r10)
	REX_STORE_U32(ctx.r10.u32 + 88, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE188) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,9
	ctx.r7.s64 = 9;
	// addi r9,r10,-18044
	ctx.r9.s64 = ctx.r10.s64 + -18044;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// addi r11,r11,-15128
	ctx.r11.s64 = ctx.r11.s64 + -15128;
	// addi r4,r10,-17996
	ctx.r4.s64 = ctx.r10.s64 + -17996;
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-29676
	ctx.r3.s64 = ctx.r10.s64 + -29676;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,3
	ctx.r10.s64 = 3;
	// li r6,44
	ctx.r6.s64 = 44;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBE1E0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE1F0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-29568
	ctx.r5.s64 = ctx.r10.s64 + -29568;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-17972
	ctx.r4.s64 = ctx.r10.s64 + -17972;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29640
	ctx.r3.s64 = ctx.r10.s64 + -29640;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,288
	ctx.r6.s64 = 288;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE23C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE250) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-29352
	ctx.r5.s64 = ctx.r10.s64 + -29352;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-17944
	ctx.r4.s64 = ctx.r10.s64 + -17944;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29604
	ctx.r3.s64 = ctx.r10.s64 + -29604;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,288
	ctx.r6.s64 = 288;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE29C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE2B0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-30108
	ctx.r5.s64 = ctx.r10.s64 + -30108;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-17920
	ctx.r4.s64 = ctx.r10.s64 + -17920;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29568
	ctx.r3.s64 = ctx.r10.s64 + -29568;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,288
	ctx.r6.s64 = 288;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE2FC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE310) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,7
	ctx.r9.s64 = 7;
	// addi r8,r10,-17856
	ctx.r8.s64 = ctx.r10.s64 + -17856;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r9,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r9.u32);
	// addi r11,r11,-17716
	ctx.r11.s64 = ctx.r11.s64 + -17716;
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// addi r4,r10,-17684
	ctx.r4.s64 = ctx.r10.s64 + -17684;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r3,r10,-29532
	ctx.r3.s64 = ctx.r10.s64 + -29532;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,68
	ctx.r6.s64 = 68;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE36C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE380) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-30108
	ctx.r5.s64 = ctx.r10.s64 + -30108;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-17592
	ctx.r4.s64 = ctx.r10.s64 + -17592;
	// addi r11,r11,-17632
	ctx.r11.s64 = ctx.r11.s64 + -17632;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29460
	ctx.r3.s64 = ctx.r10.s64 + -29460;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,304
	ctx.r6.s64 = 304;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE3D4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE3E8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-29460
	ctx.r5.s64 = ctx.r10.s64 + -29460;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-17568
	ctx.r4.s64 = ctx.r10.s64 + -17568;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29496
	ctx.r3.s64 = ctx.r10.s64 + -29496;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,304
	ctx.r6.s64 = 304;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE434;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE448) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-29460
	ctx.r5.s64 = ctx.r10.s64 + -29460;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-17552
	ctx.r4.s64 = ctx.r10.s64 + -17552;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29424
	ctx.r3.s64 = ctx.r10.s64 + -29424;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,304
	ctx.r6.s64 = 304;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE494;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE4A8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,16
	ctx.r9.s64 = 16;
	// addi r8,r10,-17288
	ctx.r8.s64 = ctx.r10.s64 + -17288;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r5,r10,-30252
	ctx.r5.s64 = ctx.r10.s64 + -30252;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r9,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r9.u32);
	// addi r11,r11,-16968
	ctx.r11.s64 = ctx.r11.s64 + -16968;
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// addi r4,r10,-20940
	ctx.r4.s64 = ctx.r10.s64 + -20940;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r3,r10,-29388
	ctx.r3.s64 = ctx.r10.s64 + -29388;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,512
	ctx.r6.s64 = 512;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE504;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE518) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-30108
	ctx.r5.s64 = ctx.r10.s64 + -30108;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-16900
	ctx.r4.s64 = ctx.r10.s64 + -16900;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29352
	ctx.r3.s64 = ctx.r10.s64 + -29352;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,288
	ctx.r6.s64 = 288;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE564;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE578) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-256(r1)
	ea = -256 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x82ab71a8
	ctx.lr = 0x82CBE590;
	sub_82AB71A8(ctx, base);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r8,-32085
	ctx.r8.s64 = -2102722560;
	// addi r10,r11,-16480
	ctx.r10.s64 = ctx.r11.s64 + -16480;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r9,-32085
	ctx.r9.s64 = -2102722560;
	// addi r11,r11,-29316
	ctx.r11.s64 = ctx.r11.s64 + -29316;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r8,27528
	ctx.r10.s64 = ctx.r8.s64 + 27528;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r9,25968
	ctx.r10.s64 = ctx.r9.s64 + 25968;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// stw r10,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r10.u32);
	// addi r1,r1,256
	ctx.r1.s64 = ctx.r1.s64 + 256;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE5D8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9104
	ctx.r3.s64 = ctx.r11.s64 + 9104;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBE5E8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-30252
	ctx.r5.s64 = ctx.r10.s64 + -30252;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-14636
	ctx.r4.s64 = ctx.r10.s64 + -14636;
	// addi r11,r11,-14676
	ctx.r11.s64 = ctx.r11.s64 + -14676;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29300
	ctx.r3.s64 = ctx.r10.s64 + -29300;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,112
	ctx.r6.s64 = 112;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE63C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE650) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-13284
	ctx.r10.s64 = ctx.r10.s64 + -13284;
	// lwz r11,-13296(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -13296);
	// stw r11,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE668) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,1
	ctx.r7.s64 = 1;
	// addi r9,r10,-12784
	ctx.r9.s64 = ctx.r10.s64 + -12784;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-11268
	ctx.r4.s64 = ctx.r10.s64 + -11268;
	// addi r11,r11,-13284
	ctx.r11.s64 = ctx.r11.s64 + -13284;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-28544
	ctx.r3.s64 = ctx.r10.s64 + -28544;
	// li r10,2
	ctx.r10.s64 = 2;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,2
	ctx.r6.s64 = 2;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE6BC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE6D0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11248
	ctx.r4.s64 = ctx.r10.s64 + -11248;
	// addi r11,r11,-12760
	ctx.r11.s64 = ctx.r11.s64 + -12760;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28652
	ctx.r3.s64 = ctx.r10.s64 + -28652;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE724;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE738) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11224
	ctx.r4.s64 = ctx.r10.s64 + -11224;
	// addi r11,r11,-12720
	ctx.r11.s64 = ctx.r11.s64 + -12720;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29264
	ctx.r3.s64 = ctx.r10.s64 + -29264;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE788;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE798) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-11208
	ctx.r4.s64 = ctx.r10.s64 + -11208;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-29228
	ctx.r3.s64 = ctx.r10.s64 + -29228;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,2
	ctx.r6.s64 = 2;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE7E4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE7F8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11180
	ctx.r4.s64 = ctx.r10.s64 + -11180;
	// addi r11,r11,-12700
	ctx.r11.s64 = ctx.r11.s64 + -12700;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28580
	ctx.r3.s64 = ctx.r10.s64 + -28580;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE84C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE860) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11152
	ctx.r4.s64 = ctx.r10.s64 + -11152;
	// addi r11,r11,-12680
	ctx.r11.s64 = ctx.r11.s64 + -12680;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28724
	ctx.r3.s64 = ctx.r10.s64 + -28724;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,144
	ctx.r6.s64 = 144;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE8B4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE8C8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11116
	ctx.r4.s64 = ctx.r10.s64 + -11116;
	// addi r11,r11,-12640
	ctx.r11.s64 = ctx.r11.s64 + -12640;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29084
	ctx.r3.s64 = ctx.r10.s64 + -29084;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE91C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE930) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11076
	ctx.r4.s64 = ctx.r10.s64 + -11076;
	// addi r11,r11,-12600
	ctx.r11.s64 = ctx.r11.s64 + -12600;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28832
	ctx.r3.s64 = ctx.r10.s64 + -28832;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,112
	ctx.r6.s64 = 112;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE984;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBE998) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11040
	ctx.r4.s64 = ctx.r10.s64 + -11040;
	// addi r11,r11,-12560
	ctx.r11.s64 = ctx.r11.s64 + -12560;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28616
	ctx.r3.s64 = ctx.r10.s64 + -28616;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// bl 0x82b647f8
	ctx.lr = 0x82CBE9EC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEA00) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-11008
	ctx.r4.s64 = ctx.r10.s64 + -11008;
	// addi r11,r11,-12540
	ctx.r11.s64 = ctx.r11.s64 + -12540;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28868
	ctx.r3.s64 = ctx.r10.s64 + -28868;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEA54;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEA68) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10988
	ctx.r4.s64 = ctx.r10.s64 + -10988;
	// addi r11,r11,-12520
	ctx.r11.s64 = ctx.r11.s64 + -12520;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28436
	ctx.r3.s64 = ctx.r10.s64 + -28436;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEABC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEAD0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10964
	ctx.r4.s64 = ctx.r10.s64 + -10964;
	// addi r11,r11,-12460
	ctx.r11.s64 = ctx.r11.s64 + -12460;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28976
	ctx.r3.s64 = ctx.r10.s64 + -28976;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEB24;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEB38) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10936
	ctx.r4.s64 = ctx.r10.s64 + -10936;
	// addi r11,r11,-12400
	ctx.r11.s64 = ctx.r11.s64 + -12400;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29048
	ctx.r3.s64 = ctx.r10.s64 + -29048;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEB8C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEBA0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10912
	ctx.r4.s64 = ctx.r10.s64 + -10912;
	// addi r11,r11,-12380
	ctx.r11.s64 = ctx.r11.s64 + -12380;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28904
	ctx.r3.s64 = ctx.r10.s64 + -28904;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEBF4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEC08) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,-12312
	ctx.r11.s64 = ctx.r11.s64 + -12312;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// addi r9,r11,100
	ctx.r9.s64 = ctx.r11.s64 + 100;
	// li r8,5
	ctx.r8.s64 = 5;
	// addi r4,r10,-10892
	ctx.r4.s64 = ctx.r10.s64 + -10892;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-29012
	ctx.r3.s64 = ctx.r10.s64 + -29012;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r9,0
	ctx.r9.s64 = 0;
	// li r8,0
	ctx.r8.s64 = 0;
	// li r6,16
	ctx.r6.s64 = 16;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEC60;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEC70) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,-12176
	ctx.r11.s64 = ctx.r11.s64 + -12176;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// addi r9,r11,-164
	ctx.r9.s64 = ctx.r11.s64 + -164;
	// li r8,6
	ctx.r8.s64 = 6;
	// addi r4,r10,-10864
	ctx.r4.s64 = ctx.r10.s64 + -10864;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-29192
	ctx.r3.s64 = ctx.r10.s64 + -29192;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r9,0
	ctx.r9.s64 = 0;
	// li r8,0
	ctx.r8.s64 = 0;
	// li r6,20
	ctx.r6.s64 = 20;
	// bl 0x82b647f8
	ctx.lr = 0x82CBECC8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBECD8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-13264
	ctx.r10.s64 = ctx.r10.s64 + -13264;
	// lwz r11,-13288(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -13288);
	// stw r11,68(r10)
	REX_STORE_U32(ctx.r10.u32 + 68, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBECF0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r9,r11,-12188
	ctx.r9.s64 = ctx.r11.s64 + -12188;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// addi r8,r9,148
	ctx.r8.s64 = ctx.r9.s64 + 148;
	// li r7,7
	ctx.r7.s64 = 7;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-10836
	ctx.r4.s64 = ctx.r10.s64 + -10836;
	// addi r11,r11,-13264
	ctx.r11.s64 = ctx.r11.s64 + -13264;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28796
	ctx.r3.s64 = ctx.r10.s64 + -28796;
	// li r10,1
	ctx.r10.s64 = 1;
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// bl 0x82b647f8
	ctx.lr = 0x82CBED4C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBED60) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10808
	ctx.r4.s64 = ctx.r10.s64 + -10808;
	// addi r11,r11,-12008
	ctx.r11.s64 = ctx.r11.s64 + -12008;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28940
	ctx.r3.s64 = ctx.r10.s64 + -28940;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEDB4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEDC8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,7
	ctx.r8.s64 = 7;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10780
	ctx.r4.s64 = ctx.r10.s64 + -10780;
	// addi r11,r11,-11928
	ctx.r11.s64 = ctx.r11.s64 + -11928;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28472
	ctx.r3.s64 = ctx.r10.s64 + -28472;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEE1C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEE30) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,5
	ctx.r8.s64 = 5;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10752
	ctx.r4.s64 = ctx.r10.s64 + -10752;
	// addi r11,r11,-11728
	ctx.r11.s64 = ctx.r11.s64 + -11728;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28508
	ctx.r3.s64 = ctx.r10.s64 + -28508;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,80
	ctx.r6.s64 = 80;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEE84;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEE98) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10720
	ctx.r4.s64 = ctx.r10.s64 + -10720;
	// addi r11,r11,-11788
	ctx.r11.s64 = ctx.r11.s64 + -11788;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28400
	ctx.r3.s64 = ctx.r10.s64 + -28400;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEEEC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEF00) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,6
	ctx.r8.s64 = 6;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10692
	ctx.r4.s64 = ctx.r10.s64 + -10692;
	// addi r11,r11,-11568
	ctx.r11.s64 = ctx.r11.s64 + -11568;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28328
	ctx.r3.s64 = ctx.r10.s64 + -28328;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEF54;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEF68) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10664
	ctx.r4.s64 = ctx.r10.s64 + -10664;
	// addi r11,r11,-11448
	ctx.r11.s64 = ctx.r11.s64 + -11448;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28364
	ctx.r3.s64 = ctx.r10.s64 + -28364;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,64
	ctx.r6.s64 = 64;
	// bl 0x82b647f8
	ctx.lr = 0x82CBEFBC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBEFD0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-28544
	ctx.r5.s64 = ctx.r10.s64 + -28544;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10640
	ctx.r4.s64 = ctx.r10.s64 + -10640;
	// addi r11,r11,-11628
	ctx.r11.s64 = ctx.r11.s64 + -11628;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28292
	ctx.r3.s64 = ctx.r10.s64 + -28292;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF024;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF038) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28292
	ctx.r5.s64 = ctx.r10.s64 + -28292;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10612
	ctx.r4.s64 = ctx.r10.s64 + -10612;
	// addi r11,r11,-11368
	ctx.r11.s64 = ctx.r11.s64 + -11368;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29120
	ctx.r3.s64 = ctx.r10.s64 + -29120;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF08C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF0A0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-28292
	ctx.r5.s64 = ctx.r10.s64 + -28292;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10576
	ctx.r4.s64 = ctx.r10.s64 + -10576;
	// addi r11,r11,-11328
	ctx.r11.s64 = ctx.r11.s64 + -11328;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28688
	ctx.r3.s64 = ctx.r10.s64 + -28688;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF0F4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF108) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-28292
	ctx.r5.s64 = ctx.r10.s64 + -28292;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-10540
	ctx.r4.s64 = ctx.r10.s64 + -10540;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-28760
	ctx.r3.s64 = ctx.r10.s64 + -28760;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,16
	ctx.r6.s64 = 16;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF154;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF168) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28292
	ctx.r5.s64 = ctx.r10.s64 + -28292;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-10500
	ctx.r4.s64 = ctx.r10.s64 + -10500;
	// addi r11,r11,-11288
	ctx.r11.s64 = ctx.r11.s64 + -11288;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-29156
	ctx.r3.s64 = ctx.r10.s64 + -29156;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF1BC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF1D0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r9,r11,-9704
	ctx.r9.s64 = ctx.r11.s64 + -9704;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r8,r9,12
	ctx.r8.s64 = ctx.r9.s64 + 12;
	// addi r5,r11,-26288
	ctx.r5.s64 = ctx.r11.s64 + -26288;
	// li r7,1
	ctx.r7.s64 = 1;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r4,r11,-9672
	ctx.r4.s64 = ctx.r11.s64 + -9672;
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r11,-28256
	ctx.r3.s64 = ctx.r11.s64 + -28256;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF228;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF238) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-9560
	ctx.r4.s64 = ctx.r10.s64 + -9560;
	// addi r11,r11,-9640
	ctx.r11.s64 = ctx.r11.s64 + -9640;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28220
	ctx.r3.s64 = ctx.r10.s64 + -28220;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,24
	ctx.r6.s64 = 24;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF28C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF2A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-13116
	ctx.r10.s64 = ctx.r10.s64 + -13116;
	// lwz r11,-13120(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -13120);
	// stw r11,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF2B8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,3
	ctx.r7.s64 = 3;
	// addi r9,r10,-9380
	ctx.r9.s64 = ctx.r10.s64 + -9380;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-9368
	ctx.r4.s64 = ctx.r10.s64 + -9368;
	// addi r11,r11,-13116
	ctx.r11.s64 = ctx.r11.s64 + -13116;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-28184
	ctx.r3.s64 = ctx.r10.s64 + -28184;
	// li r10,1
	ctx.r10.s64 = 1;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF30C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF320) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-13052
	ctx.r10.s64 = ctx.r10.s64 + -13052;
	// lwz r11,-13056(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -13056);
	// stw r11,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF338) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,1
	ctx.r7.s64 = 1;
	// addi r9,r10,-9096
	ctx.r9.s64 = ctx.r10.s64 + -9096;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// addi r11,r11,-13052
	ctx.r11.s64 = ctx.r11.s64 + -13052;
	// addi r4,r10,-9084
	ctx.r4.s64 = ctx.r10.s64 + -9084;
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-28148
	ctx.r3.s64 = ctx.r10.s64 + -28148;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// li r6,12
	ctx.r6.s64 = 12;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBF390;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF3A0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-27860
	ctx.r5.s64 = ctx.r10.s64 + -27860;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-7036
	ctx.r4.s64 = ctx.r10.s64 + -7036;
	// addi r11,r11,-6552
	ctx.r11.s64 = ctx.r11.s64 + -6552;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-28040
	ctx.r3.s64 = ctx.r10.s64 + -28040;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,112
	ctx.r6.s64 = 112;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF3F4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF408) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-27860
	ctx.r5.s64 = ctx.r10.s64 + -27860;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-6492
	ctx.r4.s64 = ctx.r10.s64 + -6492;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-28004
	ctx.r3.s64 = ctx.r10.s64 + -28004;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF454;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF468) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r4,r10,-6452
	ctx.r4.s64 = ctx.r10.s64 + -6452;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r3,r10,-27932
	ctx.r3.s64 = ctx.r10.s64 + -27932;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,1
	ctx.r8.s64 = 1;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF4B0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF4C0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27932
	ctx.r5.s64 = ctx.r10.s64 + -27932;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-8032
	ctx.r4.s64 = ctx.r10.s64 + -8032;
	// addi r11,r11,-6472
	ctx.r11.s64 = ctx.r11.s64 + -6472;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27968
	ctx.r3.s64 = ctx.r10.s64 + -27968;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF514;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF528) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r5,r10,-27860
	ctx.r5.s64 = ctx.r10.s64 + -27860;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// addi r11,r11,-6416
	ctx.r11.s64 = ctx.r11.s64 + -6416;
	// addi r4,r10,-6396
	ctx.r4.s64 = ctx.r10.s64 + -6396;
	// li r9,0
	ctx.r9.s64 = 0;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// addi r3,r10,-27896
	ctx.r3.s64 = ctx.r10.s64 + -27896;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBF578;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF588) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-6356
	ctx.r4.s64 = ctx.r10.s64 + -6356;
	// addi r11,r11,-6376
	ctx.r11.s64 = ctx.r11.s64 + -6376;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27860
	ctx.r3.s64 = ctx.r10.s64 + -27860;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF5DC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF5F0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-27860
	ctx.r5.s64 = ctx.r10.s64 + -27860;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-6348
	ctx.r4.s64 = ctx.r10.s64 + -6348;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-27824
	ctx.r3.s64 = ctx.r10.s64 + -27824;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF63C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF650) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-28004
	ctx.r5.s64 = ctx.r10.s64 + -28004;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-6300
	ctx.r4.s64 = ctx.r10.s64 + -6300;
	// addi r11,r11,-6320
	ctx.r11.s64 = ctx.r11.s64 + -6320;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27788
	ctx.r3.s64 = ctx.r10.s64 + -27788;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF6A4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF6B8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27860
	ctx.r5.s64 = ctx.r10.s64 + -27860;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-6264
	ctx.r4.s64 = ctx.r10.s64 + -6264;
	// addi r11,r11,-6284
	ctx.r11.s64 = ctx.r11.s64 + -6284;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27752
	ctx.r3.s64 = ctx.r10.s64 + -27752;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF70C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF720) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r5,r10,-27012
	ctx.r5.s64 = ctx.r10.s64 + -27012;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26716
	ctx.r4.s64 = ctx.r10.s64 + -26716;
	// addi r11,r11,-6216
	ctx.r11.s64 = ctx.r11.s64 + -6216;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27716
	ctx.r3.s64 = ctx.r10.s64 + -27716;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF774;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF788) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26740
	ctx.r4.s64 = ctx.r10.s64 + -26740;
	// addi r11,r11,-6116
	ctx.r11.s64 = ctx.r11.s64 + -6116;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27680
	ctx.r3.s64 = ctx.r10.s64 + -27680;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF7D8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF7E8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27896
	ctx.r5.s64 = ctx.r10.s64 + -27896;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-6056
	ctx.r4.s64 = ctx.r10.s64 + -6056;
	// addi r11,r11,-6076
	ctx.r11.s64 = ctx.r11.s64 + -6076;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27644
	ctx.r3.s64 = ctx.r10.s64 + -27644;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF83C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF850) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,3
	ctx.r8.s64 = 3;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26776
	ctx.r4.s64 = ctx.r10.s64 + -26776;
	// addi r11,r11,-5968
	ctx.r11.s64 = ctx.r11.s64 + -5968;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27572
	ctx.r3.s64 = ctx.r10.s64 + -27572;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF8A0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF8B0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,5
	ctx.r8.s64 = 5;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-5804
	ctx.r4.s64 = ctx.r10.s64 + -5804;
	// addi r11,r11,-5904
	ctx.r11.s64 = ctx.r11.s64 + -5904;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27608
	ctx.r3.s64 = ctx.r10.s64 + -27608;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,80
	ctx.r6.s64 = 80;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF904;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF918) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,3
	ctx.r8.s64 = 3;
	// addi r5,r10,-27500
	ctx.r5.s64 = ctx.r10.s64 + -27500;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26792
	ctx.r4.s64 = ctx.r10.s64 + -26792;
	// addi r11,r11,-5736
	ctx.r11.s64 = ctx.r11.s64 + -5736;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27536
	ctx.r3.s64 = ctx.r10.s64 + -27536;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,36
	ctx.r6.s64 = 36;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF96C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF980) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,4
	ctx.r8.s64 = 4;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26804
	ctx.r4.s64 = ctx.r10.s64 + -26804;
	// addi r11,r11,-5656
	ctx.r11.s64 = ctx.r11.s64 + -5656;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27500
	ctx.r3.s64 = ctx.r10.s64 + -27500;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBF9D0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBF9E0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r4,r10,-8208
	ctx.r4.s64 = ctx.r10.s64 + -8208;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// addi r3,r10,-27464
	ctx.r3.s64 = ctx.r10.s64 + -27464;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,16
	ctx.r6.s64 = 16;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFA2C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFA40) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27752
	ctx.r5.s64 = ctx.r10.s64 + -27752;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-5556
	ctx.r4.s64 = ctx.r10.s64 + -5556;
	// addi r11,r11,-5576
	ctx.r11.s64 = ctx.r11.s64 + -5576;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27428
	ctx.r3.s64 = ctx.r10.s64 + -27428;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,24
	ctx.r6.s64 = 24;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFA94;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFAA8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-5484
	ctx.r4.s64 = ctx.r10.s64 + -5484;
	// addi r11,r11,-5524
	ctx.r11.s64 = ctx.r11.s64 + -5524;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27392
	ctx.r3.s64 = ctx.r10.s64 + -27392;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFAFC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFB10) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-5420
	ctx.r4.s64 = ctx.r10.s64 + -5420;
	// addi r11,r11,-5460
	ctx.r11.s64 = ctx.r11.s64 + -5460;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27356
	ctx.r3.s64 = ctx.r10.s64 + -27356;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,96
	ctx.r6.s64 = 96;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFB64;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFB78) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r9,r11,-5048
	ctx.r9.s64 = ctx.r11.s64 + -5048;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// addi r8,r9,12
	ctx.r8.s64 = ctx.r9.s64 + 12;
	// li r7,6
	ctx.r7.s64 = 6;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-4888
	ctx.r4.s64 = ctx.r10.s64 + -4888;
	// addi r11,r11,-5008
	ctx.r11.s64 = ctx.r11.s64 + -5008;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27320
	ctx.r3.s64 = ctx.r10.s64 + -27320;
	// li r10,1
	ctx.r10.s64 = 1;
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,96
	ctx.r6.s64 = 96;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFBD4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFBE8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r9,r10,-4800
	ctx.r9.s64 = ctx.r10.s64 + -4800;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// addi r11,r11,-4788
	ctx.r11.s64 = ctx.r11.s64 + -4788;
	// addi r4,r10,-4748
	ctx.r4.s64 = ctx.r10.s64 + -4748;
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-27284
	ctx.r3.s64 = ctx.r10.s64 + -27284;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// li r6,48
	ctx.r6.s64 = 48;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// bl 0x82b647f8
	ctx.lr = 0x82CBFC40;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFC50) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27788
	ctx.r5.s64 = ctx.r10.s64 + -27788;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-7592
	ctx.r4.s64 = ctx.r10.s64 + -7592;
	// addi r11,r11,-4720
	ctx.r11.s64 = ctx.r11.s64 + -4720;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27248
	ctx.r3.s64 = ctx.r10.s64 + -27248;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFCA4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFCB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r31,-32048
	ctx.r31.s64 = -2100297728;
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f0,-11896(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + -11896);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,-12072(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12072);
	ctx.f13.f64 = double(temp.f32);
	// fcmpu cr6,f0,f13
	ctx.cr6.compare(ctx.f0.f64, ctx.f13.f64);
	// bge cr6,0x82cbfcf4
	if (!ctx.cr6.lt) goto loc_82CBFCF4;
	// bl 0x82afa090
	ctx.lr = 0x82CBFCE4;
	sub_82AFA090(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,2992(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f0,f0,f1
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f1.f64));
	// stfs f0,-11896(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + -11896, temp.u32);
loc_82CBFCF4:
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r7,-32080
	ctx.r7.s64 = -2102394880;
	// addi r10,r11,-4888
	ctx.r10.s64 = ctx.r11.s64 + -4888;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r8,-32080
	ctx.r8.s64 = -2102394880;
	// addi r9,r11,-4044
	ctx.r9.s64 = ctx.r11.s64 + -4044;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r11,r11,-27212
	ctx.r11.s64 = ctx.r11.s64 + -27212;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r7,-22312
	ctx.r10.s64 = ctx.r7.s64 + -22312;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r8,-24696
	ctx.r10.s64 = ctx.r8.s64 + -24696;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// stw r9,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r9.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFD40) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r8,3
	ctx.r8.s64 = 3;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-3680
	ctx.r4.s64 = ctx.r10.s64 + -3680;
	// addi r11,r11,-3824
	ctx.r11.s64 = ctx.r11.s64 + -3824;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27196
	ctx.r3.s64 = ctx.r10.s64 + -27196;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFD90;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFDA0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,4
	ctx.r8.s64 = 4;
	// addi r5,r10,-27896
	ctx.r5.s64 = ctx.r10.s64 + -27896;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-3652
	ctx.r4.s64 = ctx.r10.s64 + -3652;
	// addi r11,r11,-3760
	ctx.r11.s64 = ctx.r11.s64 + -3760;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27160
	ctx.r3.s64 = ctx.r10.s64 + -27160;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,60
	ctx.r6.s64 = 60;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFDF4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFE08) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9128
	ctx.r3.s64 = ctx.r11.s64 + 9128;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CBFE18) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26588
	ctx.r4.s64 = ctx.r10.s64 + -26588;
	// addi r11,r11,1400
	ctx.r11.s64 = ctx.r11.s64 + 1400;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27084
	ctx.r3.s64 = ctx.r10.s64 + -27084;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFE68;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFE78) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-26288
	ctx.r5.s64 = ctx.r10.s64 + -26288;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,1460
	ctx.r4.s64 = ctx.r10.s64 + 1460;
	// addi r11,r11,1420
	ctx.r11.s64 = ctx.r11.s64 + 1420;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27120
	ctx.r3.s64 = ctx.r10.s64 + -27120;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFECC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFEE0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,1
	ctx.r8.s64 = 1;
	// addi r5,r10,-27536
	ctx.r5.s64 = ctx.r10.s64 + -27536;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26608
	ctx.r4.s64 = ctx.r10.s64 + -26608;
	// addi r11,r11,1492
	ctx.r11.s64 = ctx.r11.s64 + 1492;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27048
	ctx.r3.s64 = ctx.r10.s64 + -27048;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,48
	ctx.r6.s64 = 48;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFF34;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFF48) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26628
	ctx.r4.s64 = ctx.r10.s64 + -26628;
	// addi r11,r11,1512
	ctx.r11.s64 = ctx.r11.s64 + 1512;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-27012
	ctx.r3.s64 = ctx.r10.s64 + -27012;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFF98;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CBFFA8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r9,r11,1740
	ctx.r9.s64 = ctx.r11.s64 + 1740;
	// li r7,7
	ctx.r7.s64 = 7;
	// addi r8,r9,12
	ctx.r8.s64 = ctx.r9.s64 + 12;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// li r6,304
	ctx.r6.s64 = 304;
	// addi r4,r11,1912
	ctx.r4.s64 = ctx.r11.s64 + 1912;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r3,r11,-26940
	ctx.r3.s64 = ctx.r11.s64 + -26940;
	// li r8,0
	ctx.r8.s64 = 0;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CBFFFC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0010) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r9,r11,1704
	ctx.r9.s64 = ctx.r11.s64 + 1704;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r8,r9,188
	ctx.r8.s64 = ctx.r9.s64 + 188;
	// addi r5,r11,-28256
	ctx.r5.s64 = ctx.r11.s64 + -28256;
	// li r7,1
	ctx.r7.s64 = 1;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r4,r11,1944
	ctx.r4.s64 = ctx.r11.s64 + 1944;
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r11,-26976
	ctx.r3.s64 = ctx.r11.s64 + -26976;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,320
	ctx.r6.s64 = 320;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0068;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0078) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f0,-12072(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// stfs f0,-16(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -16, temp.u32);
	// stfs f0,-12(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -12, temp.u32);
	// stfs f0,-8(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -8, temp.u32);
	// lfs f0,2992(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 2992);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stfs f0,-4(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -4, temp.u32);
	// addi r10,r1,-16
	ctx.r10.s64 = ctx.r1.s64 + -16;
	// addi r11,r11,-26896
	ctx.r11.s64 = ctx.r11.s64 + -26896;
	// lvx128 v0,r0,r10
	ea = (ctx.r10.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)REX_RAW_ADDR(ea)), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// stvx128 v0,r0,r11
	ea = (ctx.r11.u32) & ~0xF;
	simde_mm_store_si128((simde__m128i*)REX_RAW_ADDR(ea), simde_mm_shuffle_epi8(simde_mm_load_si128((simde__m128i*)ctx.v0.u8), simde_mm_load_si128((simde__m128i*)VectorMaskL)));
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC00B0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,-26880
	ctx.r11.s64 = ctx.r11.s64 + -26880;
	// addi r10,r10,-26864
	ctx.r10.s64 = ctx.r10.s64 + -26864;
	// ld r9,0(r11)
	ctx.r9.u64 = REX_LOAD_U64(ctx.r11.u32 + 0);
	// ld r11,8(r11)
	ctx.r11.u64 = REX_LOAD_U64(ctx.r11.u32 + 8);
	// std r9,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.r9.u64);
	// std r11,8(r10)
	REX_STORE_U64(ctx.r10.u32 + 8, ctx.r11.u64);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC00D8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,5
	ctx.r8.s64 = 5;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26980
	ctx.r4.s64 = ctx.r10.s64 + -26980;
	// addi r11,r11,4480
	ctx.r11.s64 = ctx.r11.s64 + 4480;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26848
	ctx.r3.s64 = ctx.r10.s64 + -26848;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,80
	ctx.r6.s64 = 80;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0128;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0138) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-26992
	ctx.r4.s64 = ctx.r10.s64 + -26992;
	// addi r11,r11,4580
	ctx.r11.s64 = ctx.r11.s64 + 4580;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26812
	ctx.r3.s64 = ctx.r10.s64 + -26812;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0188;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0198) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,11
	ctx.r8.s64 = 11;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27008
	ctx.r4.s64 = ctx.r10.s64 + -27008;
	// addi r11,r11,4784
	ctx.r11.s64 = ctx.r11.s64 + 4784;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26776
	ctx.r3.s64 = ctx.r10.s64 + -26776;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,192
	ctx.r6.s64 = 192;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC01E8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC01F8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r9,r10,5104
	ctx.r9.s64 = ctx.r10.s64 + 5104;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27032
	ctx.r4.s64 = ctx.r10.s64 + -27032;
	// addi r11,r11,5120
	ctx.r11.s64 = ctx.r11.s64 + 5120;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26740
	ctx.r3.s64 = ctx.r10.s64 + -26740;
	// li r10,1
	ctx.r10.s64 = 1;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC024C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0260) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27048
	ctx.r4.s64 = ctx.r10.s64 + -27048;
	// addi r11,r11,5220
	ctx.r11.s64 = ctx.r11.s64 + 5220;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26704
	ctx.r3.s64 = ctx.r10.s64 + -26704;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC02B0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC02C0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27056
	ctx.r4.s64 = ctx.r10.s64 + -27056;
	// addi r11,r11,5260
	ctx.r11.s64 = ctx.r11.s64 + 5260;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26668
	ctx.r3.s64 = ctx.r10.s64 + -26668;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,32
	ctx.r6.s64 = 32;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0310;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0320) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// bl 0x82cc2f14
	ctx.lr = 0x82CC0330;
	__imp__KeTlsAlloc(ctx, base);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// stw r3,-26624(r10)
	REX_STORE_U32(ctx.r10.u32 + -26624, ctx.r3.u32);
	// addi r3,r11,9152
	ctx.r3.s64 = ctx.r11.s64 + 9152;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0344;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0358) {
	REX_FUNC_PROLOGUE();
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,-8072
	ctx.r11.s64 = ctx.r11.s64 + -8072;
	// lwz r9,-26200(r10)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r10.u32 + -26200);
	// stw r11,-26200(r10)
	REX_STORE_U32(ctx.r10.u32 + -26200, ctx.r11.u32);
	// stw r9,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0378) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,3
	ctx.r7.s64 = 3;
	// addi r9,r10,6108
	ctx.r9.s64 = ctx.r10.s64 + 6108;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27076
	ctx.r4.s64 = ctx.r10.s64 + -27076;
	// addi r11,r11,6132
	ctx.r11.s64 = ctx.r11.s64 + 6132;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26612
	ctx.r3.s64 = ctx.r10.s64 + -26612;
	// li r10,2
	ctx.r10.s64 = 2;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC03CC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC03E0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27112
	ctx.r4.s64 = ctx.r10.s64 + -27112;
	// addi r11,r11,6348
	ctx.r11.s64 = ctx.r11.s64 + 6348;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26576
	ctx.r3.s64 = ctx.r10.s64 + -26576;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,16
	ctx.r6.s64 = 16;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0430;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0440) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27140
	ctx.r4.s64 = ctx.r10.s64 + -27140;
	// addi r11,r11,6388
	ctx.r11.s64 = ctx.r11.s64 + 6388;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26504
	ctx.r3.s64 = ctx.r10.s64 + -26504;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0490;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC04A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-8040
	ctx.r10.s64 = ctx.r10.s64 + -8040;
	// lwz r11,-8048(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -8048);
	// stw r11,68(r10)
	REX_STORE_U32(ctx.r10.u32 + 68, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC04B8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,6
	ctx.r7.s64 = 6;
	// addi r9,r10,6432
	ctx.r9.s64 = ctx.r10.s64 + 6432;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-27168
	ctx.r4.s64 = ctx.r10.s64 + -27168;
	// addi r11,r11,-8040
	ctx.r11.s64 = ctx.r11.s64 + -8040;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26540
	ctx.r3.s64 = ctx.r10.s64 + -26540;
	// li r10,1
	ctx.r10.s64 = 1;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,24
	ctx.r6.s64 = 24;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC050C;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0520) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-7904
	ctx.r10.s64 = ctx.r10.s64 + -7904;
	// lwz r11,-7920(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -7920);
	// stw r11,68(r10)
	REX_STORE_U32(ctx.r10.u32 + 68, ctx.r11.u32);
	// stw r11,88(r10)
	REX_STORE_U32(ctx.r10.u32 + 88, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0540) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,8
	ctx.r7.s64 = 8;
	// addi r9,r10,7448
	ctx.r9.s64 = ctx.r10.s64 + 7448;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-27184
	ctx.r4.s64 = ctx.r10.s64 + -27184;
	// addi r11,r11,-7904
	ctx.r11.s64 = ctx.r11.s64 + -7904;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26468
	ctx.r3.s64 = ctx.r10.s64 + -26468;
	// li r10,3
	ctx.r10.s64 = 3;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0594;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC05A8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27200
	ctx.r4.s64 = ctx.r10.s64 + -27200;
	// addi r11,r11,7512
	ctx.r11.s64 = ctx.r11.s64 + 7512;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26432
	ctx.r3.s64 = ctx.r10.s64 + -26432;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC05F8;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0608) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,2
	ctx.r8.s64 = 2;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27212
	ctx.r4.s64 = ctx.r10.s64 + -27212;
	// addi r11,r11,7552
	ctx.r11.s64 = ctx.r11.s64 + 7552;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26396
	ctx.r3.s64 = ctx.r10.s64 + -26396;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,12
	ctx.r6.s64 = 12;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0658;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0668) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,7
	ctx.r7.s64 = 7;
	// addi r9,r10,7688
	ctx.r9.s64 = ctx.r10.s64 + 7688;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27220
	ctx.r4.s64 = ctx.r10.s64 + -27220;
	// addi r11,r11,7704
	ctx.r11.s64 = ctx.r11.s64 + 7704;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26360
	ctx.r3.s64 = ctx.r10.s64 + -26360;
	// li r10,1
	ctx.r10.s64 = 1;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,36
	ctx.r6.s64 = 36;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC06BC;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC06D0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,1
	ctx.r8.s64 = 1;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27232
	ctx.r4.s64 = ctx.r10.s64 + -27232;
	// addi r11,r11,7852
	ctx.r11.s64 = ctx.r11.s64 + 7852;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26324
	ctx.r3.s64 = ctx.r10.s64 + -26324;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0720;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0730) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,2
	ctx.r8.s64 = 2;
	// addi r5,r10,-26252
	ctx.r5.s64 = ctx.r10.s64 + -26252;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,7944
	ctx.r4.s64 = ctx.r10.s64 + 7944;
	// addi r11,r11,7904
	ctx.r11.s64 = ctx.r11.s64 + 7904;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26288
	ctx.r3.s64 = ctx.r10.s64 + -26288;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,8
	ctx.r6.s64 = 8;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0784;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0798) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r11,0
	ctx.r11.s64 = 0;
	// addi r4,r10,7964
	ctx.r4.s64 = ctx.r10.s64 + 7964;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r3,r10,-26252
	ctx.r3.s64 = ctx.r10.s64 + -26252;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r11.u32);
	// li r8,1
	ctx.r8.s64 = 1;
	// stw r11,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,4
	ctx.r6.s64 = 4;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC07E0;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC07F0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// li r8,7
	ctx.r8.s64 = 7;
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// addi r4,r10,-27220
	ctx.r4.s64 = ctx.r10.s64 + -27220;
	// addi r11,r11,7984
	ctx.r11.s64 = ctx.r11.s64 + 7984;
	// li r9,0
	ctx.r9.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// addi r3,r10,-26196
	ctx.r3.s64 = ctx.r10.s64 + -26196;
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r7,0
	ctx.r7.s64 = 0;
	// stw r9,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r9.u32);
	// li r6,36
	ctx.r6.s64 = 36;
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC0840;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0850) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r10,r10,-7688
	ctx.r10.s64 = ctx.r10.s64 + -7688;
	// lwz r11,-7704(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + -7704);
	// stw r11,68(r10)
	REX_STORE_U32(ctx.r10.u32 + 68, ctx.r11.u32);
	// stw r11,88(r10)
	REX_STORE_U32(ctx.r10.u32 + 88, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0870) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r10,-32230
	ctx.r10.s64 = -2112225280;
	// li r7,8
	ctx.r7.s64 = 8;
	// addi r9,r10,8124
	ctx.r9.s64 = ctx.r10.s64 + 8124;
	// lis r10,-32234
	ctx.r10.s64 = -2112487424;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r4,r10,-27184
	ctx.r4.s64 = ctx.r10.s64 + -27184;
	// addi r11,r11,-7688
	ctx.r11.s64 = ctx.r11.s64 + -7688;
	// stw r7,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r7.u32);
	// li r8,0
	ctx.r8.s64 = 0;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r3,r10,-26160
	ctx.r3.s64 = ctx.r10.s64 + -26160;
	// li r10,3
	ctx.r10.s64 = 3;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// li r6,20
	ctx.r6.s64 = 20;
	// stw r8,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r8.u32);
	// li r5,0
	ctx.r5.s64 = 0;
	// bl 0x82b647f8
	ctx.lr = 0x82CC08C4;
	sub_82B647F8(ctx, base);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC08D8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// bl 0x82cc2f14
	ctx.lr = 0x82CC08E8;
	__imp__KeTlsAlloc(ctx, base);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// stw r3,-26120(r10)
	REX_STORE_U32(ctx.r10.u32 + -26120, ctx.r3.u32);
	// addi r3,r11,9168
	ctx.r3.s64 = ctx.r11.s64 + 9168;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC08FC;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0910) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// bl 0x82cc2f14
	ctx.lr = 0x82CC0920;
	__imp__KeTlsAlloc(ctx, base);
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// stw r3,-26112(r10)
	REX_STORE_U32(ctx.r10.u32 + -26112, ctx.r3.u32);
	// addi r3,r11,9200
	ctx.r3.s64 = ctx.r11.s64 + 9200;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0934;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0948) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// li r5,1
	ctx.r5.s64 = 1;
	// addi r3,r11,-26108
	ctx.r3.s64 = ctx.r11.s64 + -26108;
	// li r4,1
	ctx.r4.s64 = 1;
	// bl 0x82b6c8f0
	ctx.lr = 0x82CC0968;
	sub_82B6C8F0(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9184
	ctx.r3.s64 = ctx.r11.s64 + 9184;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0974;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0988) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9216
	ctx.r3.s64 = ctx.r11.s64 + 9216;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0998) {
	REX_FUNC_PROLOGUE();
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,-7392
	ctx.r11.s64 = ctx.r11.s64 + -7392;
	// lwz r9,-26200(r10)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r10.u32 + -26200);
	// stw r11,-26200(r10)
	REX_STORE_U32(ctx.r10.u32 + -26200, ctx.r11.u32);
	// stw r9,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r9.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC09B8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,-5888
	ctx.r11.s64 = ctx.r11.s64 + -5888;
	// addi r3,r11,4
	ctx.r3.s64 = ctx.r11.s64 + 4;
	// bl 0x82cc2894
	ctx.lr = 0x82CC09D4;
	__imp__RtlInitializeCriticalSection(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9224
	ctx.r3.s64 = ctx.r11.s64 + 9224;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC09E0;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC09F0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,5184
	ctx.r11.s64 = ctx.r11.s64 + 5184;
	// addi r3,r11,4
	ctx.r3.s64 = ctx.r11.s64 + 4;
	// bl 0x82cc2894
	ctx.lr = 0x82CC0A0C;
	__imp__RtlInitializeCriticalSection(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9248
	ctx.r3.s64 = ctx.r11.s64 + 9248;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0A18;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0A28) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r11,r11,5216
	ctx.r11.s64 = ctx.r11.s64 + 5216;
	// addi r3,r11,4
	ctx.r3.s64 = ctx.r11.s64 + 4;
	// bl 0x82cc2894
	ctx.lr = 0x82CC0A44;
	__imp__RtlInitializeCriticalSection(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9272
	ctx.r3.s64 = ctx.r11.s64 + 9272;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0A50;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0A60) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5300(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5300);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stfs f0,-9668(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9668, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0A78) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5300(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5300);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9672(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9672, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0A98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5300(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5300);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9660(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9660, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0AB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5300(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5300);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9664(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9664, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0AD8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5324(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5324);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stfs f0,-9648(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9648, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0AF0) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5324(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5324);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9652(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9652, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0B10) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5324(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5324);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9640(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9640, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0B30) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5324(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5324);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9644(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9644, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0B50) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9296
	ctx.r3.s64 = ctx.r11.s64 + 9296;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0B60) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stfs f0,-9396(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9396, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0B78) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9400(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9400, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0B98) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9388(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9388, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0BB8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,5476(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5476);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,-9392(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + -9392, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0BD8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9312
	ctx.r3.s64 = ctx.r11.s64 + 9312;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0BE8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9320
	ctx.r3.s64 = ctx.r11.s64 + 9320;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0BF8) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r9,-32226
	ctx.r9.s64 = -2111963136;
	// addi r11,r11,-4976
	ctx.r11.s64 = ctx.r11.s64 + -4976;
	// lfs f0,-12072(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -12072);
	ctx.f0.f64 = double(temp.f32);
	// lwz r10,64(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 64);
	// stfs f0,76(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 76, temp.u32);
	// lwz r9,144(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 144);
	// stfs f0,156(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 156, temp.u32);
	// lwz r8,224(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 224);
	// stfs f0,236(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 236, temp.u32);
	// lwz r7,304(r11)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r11.u32 + 304);
	// stfs f0,316(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 316, temp.u32);
	// oris r10,r10,32768
	ctx.r10.u64 = ctx.r10.u64 | 2147483648;
	// lwz r6,384(r11)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r11.u32 + 384);
	// stfs f0,396(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 396, temp.u32);
	// lwz r5,464(r11)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r11.u32 + 464);
	// rlwinm r10,r10,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stfs f0,476(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 476, temp.u32);
	// lwz r4,544(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 544);
	// stfs f0,556(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 556, temp.u32);
	// lwz r3,624(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 624);
	// stfs f0,636(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 636, temp.u32);
	// oris r9,r9,32768
	ctx.r9.u64 = ctx.r9.u64 | 2147483648;
	// oris r8,r8,32768
	ctx.r8.u64 = ctx.r8.u64 | 2147483648;
	// stw r10,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// oris r7,r7,32768
	ctx.r7.u64 = ctx.r7.u64 | 2147483648;
	// oris r6,r6,32768
	ctx.r6.u64 = ctx.r6.u64 | 2147483648;
	// oris r5,r5,32768
	ctx.r5.u64 = ctx.r5.u64 | 2147483648;
	// oris r4,r4,32768
	ctx.r4.u64 = ctx.r4.u64 | 2147483648;
	// stw r10,72(r11)
	REX_STORE_U32(ctx.r11.u32 + 72, ctx.r10.u32);
	// rlwinm r10,r9,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// oris r3,r3,32768
	ctx.r3.u64 = ctx.r3.u64 | 2147483648;
	// stw r10,144(r11)
	REX_STORE_U32(ctx.r11.u32 + 144, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,152(r11)
	REX_STORE_U32(ctx.r11.u32 + 152, ctx.r10.u32);
	// rlwinm r10,r8,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,232(r11)
	REX_STORE_U32(ctx.r11.u32 + 232, ctx.r10.u32);
	// rlwinm r10,r7,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,304(r11)
	REX_STORE_U32(ctx.r11.u32 + 304, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,312(r11)
	REX_STORE_U32(ctx.r11.u32 + 312, ctx.r10.u32);
	// rlwinm r10,r6,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,384(r11)
	REX_STORE_U32(ctx.r11.u32 + 384, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,392(r11)
	REX_STORE_U32(ctx.r11.u32 + 392, ctx.r10.u32);
	// rlwinm r10,r5,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,464(r11)
	REX_STORE_U32(ctx.r11.u32 + 464, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,472(r11)
	REX_STORE_U32(ctx.r11.u32 + 472, ctx.r10.u32);
	// rlwinm r10,r4,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,544(r11)
	REX_STORE_U32(ctx.r11.u32 + 544, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,552(r11)
	REX_STORE_U32(ctx.r11.u32 + 552, ctx.r10.u32);
	// rlwinm r10,r3,0,3,0
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 0) & 0xFFFFFFFF9FFFFFFF;
	// stw r10,624(r11)
	REX_STORE_U32(ctx.r11.u32 + 624, ctx.r10.u32);
	// li r10,0
	ctx.r10.s64 = 0;
	// stw r10,632(r11)
	REX_STORE_U32(ctx.r11.u32 + 632, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0CF0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,160
	ctx.r5.s64 = 160;
	// addi r3,r11,5556
	ctx.r3.s64 = ctx.r11.s64 + 5556;
	// li r4,0
	ctx.r4.s64 = 0;
	// bl 0x82a3c440
	ctx.lr = 0x82CC0D10;
	sub_82A3C440(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9408
	ctx.r3.s64 = ctx.r11.s64 + 9408;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0D1C;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0D30) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9328
	ctx.r3.s64 = ctx.r11.s64 + 9328;
	// b 0x82a3cc28
	sub_82A3CC28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0D40) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r3,r11,6136
	ctx.r3.s64 = ctx.r11.s64 + 6136;
	// bl 0x82ca9900
	ctx.lr = 0x82CC0D58;
	sub_82CA9900(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9336
	ctx.r3.s64 = ctx.r11.s64 + 9336;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC0D64;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0D78) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r31,r11,6312
	ctx.r31.s64 = ctx.r11.s64 + 6312;
	// li r10,7
	ctx.r10.s64 = 7;
	// addi r11,r31,52
	ctx.r11.s64 = ctx.r31.s64 + 52;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_82CC0DA0:
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x82cc0da0
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_82CC0DA0;
	// li r5,512
	ctx.r5.s64 = 512;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,80
	ctx.r3.s64 = ctx.r31.s64 + 80;
	// bl 0x82a3c440
	ctx.lr = 0x82CC0DBC;
	sub_82A3C440(ctx, base);
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,592(r31)
	REX_STORE_U32(ctx.r31.u32 + 592, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0DD8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r11,r11,6960
	ctx.r11.s64 = ctx.r11.s64 + 6960;
	// li r10,6
	ctx.r10.s64 = 6;
	// addi r11,r11,52
	ctx.r11.s64 = ctx.r11.s64 + 52;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_82CC0DF0:
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x82cc0df0
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_82CC0DF0;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0E00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,7040
	ctx.r11.s64 = ctx.r11.s64 + 7040;
	// stw r10,108(r11)
	REX_STORE_U32(ctx.r11.u32 + 108, ctx.r10.u32);
	// stw r10,112(r11)
	REX_STORE_U32(ctx.r11.u32 + 112, ctx.r10.u32);
	// stw r10,116(r11)
	REX_STORE_U32(ctx.r11.u32 + 116, ctx.r10.u32);
	// stw r10,120(r11)
	REX_STORE_U32(ctx.r11.u32 + 120, ctx.r10.u32);
	// stw r10,124(r11)
	REX_STORE_U32(ctx.r11.u32 + 124, ctx.r10.u32);
	// stw r10,140(r11)
	REX_STORE_U32(ctx.r11.u32 + 140, ctx.r10.u32);
	// stw r10,144(r11)
	REX_STORE_U32(ctx.r11.u32 + 144, ctx.r10.u32);
	// stw r10,148(r11)
	REX_STORE_U32(ctx.r11.u32 + 148, ctx.r10.u32);
	// stb r10,152(r11)
	REX_STORE_U8(ctx.r11.u32 + 152, ctx.r10.u8);
	// stb r10,153(r11)
	REX_STORE_U8(ctx.r11.u32 + 153, ctx.r10.u8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0E38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,7196
	ctx.r11.s64 = ctx.r11.s64 + 7196;
	// stw r10,124(r11)
	REX_STORE_U32(ctx.r11.u32 + 124, ctx.r10.u32);
	// stw r10,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r10.u32);
	// stw r10,132(r11)
	REX_STORE_U32(ctx.r11.u32 + 132, ctx.r10.u32);
	// stw r10,136(r11)
	REX_STORE_U32(ctx.r11.u32 + 136, ctx.r10.u32);
	// stw r10,140(r11)
	REX_STORE_U32(ctx.r11.u32 + 140, ctx.r10.u32);
	// stb r10,120(r11)
	REX_STORE_U8(ctx.r11.u32 + 120, ctx.r10.u8);
	// stw r10,44(r11)
	REX_STORE_U32(ctx.r11.u32 + 44, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0E68) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,7340
	ctx.r11.s64 = ctx.r11.s64 + 7340;
	// stw r10,52(r11)
	REX_STORE_U32(ctx.r11.u32 + 52, ctx.r10.u32);
	// stw r10,56(r11)
	REX_STORE_U32(ctx.r11.u32 + 56, ctx.r10.u32);
	// stw r10,60(r11)
	REX_STORE_U32(ctx.r11.u32 + 60, ctx.r10.u32);
	// stw r10,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r10.u32);
	// stw r10,68(r11)
	REX_STORE_U32(ctx.r11.u32 + 68, ctx.r10.u32);
	// stb r10,48(r11)
	REX_STORE_U8(ctx.r11.u32 + 48, ctx.r10.u8);
	// stw r10,72(r11)
	REX_STORE_U32(ctx.r11.u32 + 72, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0E98) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,7416
	ctx.r11.s64 = ctx.r11.s64 + 7416;
	// stw r10,108(r11)
	REX_STORE_U32(ctx.r11.u32 + 108, ctx.r10.u32);
	// stw r10,112(r11)
	REX_STORE_U32(ctx.r11.u32 + 112, ctx.r10.u32);
	// stw r10,116(r11)
	REX_STORE_U32(ctx.r11.u32 + 116, ctx.r10.u32);
	// stw r10,120(r11)
	REX_STORE_U32(ctx.r11.u32 + 120, ctx.r10.u32);
	// stw r10,124(r11)
	REX_STORE_U32(ctx.r11.u32 + 124, ctx.r10.u32);
	// stw r10,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r10.u32);
	// stb r10,144(r11)
	REX_STORE_U8(ctx.r11.u32 + 144, ctx.r10.u8);
	// stb r10,145(r11)
	REX_STORE_U8(ctx.r11.u32 + 145, ctx.r10.u8);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0EC8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r30,-24(r1)
	REX_STORE_U64(ctx.r1.u32 + -24, ctx.r30.u64);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r30,0
	ctx.r30.s64 = 0;
	// addi r31,r11,7564
	ctx.r31.s64 = ctx.r11.s64 + 7564;
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// li r5,52
	ctx.r5.s64 = 52;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,92
	ctx.r3.s64 = ctx.r31.s64 + 92;
	// stw r30,52(r31)
	REX_STORE_U32(ctx.r31.u32 + 52, ctx.r30.u32);
	// stw r30,56(r31)
	REX_STORE_U32(ctx.r31.u32 + 56, ctx.r30.u32);
	// stw r30,60(r31)
	REX_STORE_U32(ctx.r31.u32 + 60, ctx.r30.u32);
	// stw r30,64(r31)
	REX_STORE_U32(ctx.r31.u32 + 64, ctx.r30.u32);
	// stw r30,68(r31)
	REX_STORE_U32(ctx.r31.u32 + 68, ctx.r30.u32);
	// stw r11,72(r31)
	REX_STORE_U32(ctx.r31.u32 + 72, ctx.r11.u32);
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r11,76(r31)
	REX_STORE_U32(ctx.r31.u32 + 76, ctx.r11.u32);
	// bl 0x82a3c440
	ctx.lr = 0x82CC0F1C;
	sub_82A3C440(ctx, base);
	// li r11,12
	ctx.r11.s64 = 12;
	// stw r11,144(r31)
	REX_STORE_U32(ctx.r31.u32 + 144, ctx.r11.u32);
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// stw r11,148(r31)
	REX_STORE_U32(ctx.r31.u32 + 148, ctx.r11.u32);
	// stw r11,152(r31)
	REX_STORE_U32(ctx.r31.u32 + 152, ctx.r11.u32);
	// stw r11,156(r31)
	REX_STORE_U32(ctx.r31.u32 + 156, ctx.r11.u32);
	// stw r11,160(r31)
	REX_STORE_U32(ctx.r31.u32 + 160, ctx.r11.u32);
	// stw r11,164(r31)
	REX_STORE_U32(ctx.r31.u32 + 164, ctx.r11.u32);
	// stw r11,168(r31)
	REX_STORE_U32(ctx.r31.u32 + 168, ctx.r11.u32);
	// stb r11,244(r31)
	REX_STORE_U8(ctx.r31.u32 + 244, ctx.r11.u8);
	// li r11,1
	ctx.r11.s64 = 1;
	// stb r30,245(r31)
	REX_STORE_U8(ctx.r31.u32 + 245, ctx.r30.u8);
	// stb r11,246(r31)
	REX_STORE_U8(ctx.r31.u32 + 246, ctx.r11.u8);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r30,-24(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -24);
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0F68) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,72
	ctx.r5.s64 = 72;
	// addi r31,r11,8088
	ctx.r31.s64 = ctx.r11.s64 + 8088;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,44
	ctx.r3.s64 = ctx.r31.s64 + 44;
	// bl 0x82a3c440
	ctx.lr = 0x82CC0F90;
	sub_82A3C440(ctx, base);
	// li r9,0
	ctx.r9.s64 = 0;
	// li r10,9
	ctx.r10.s64 = 9;
	// addi r11,r31,116
	ctx.r11.s64 = ctx.r31.s64 + 116;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_82CC0FA0:
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x82cc0fa0
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_82CC0FA0;
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,152(r31)
	REX_STORE_U32(ctx.r31.u32 + 152, ctx.r11.u32);
	// stw r11,156(r31)
	REX_STORE_U32(ctx.r31.u32 + 156, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC0FD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,72
	ctx.r5.s64 = 72;
	// addi r11,r11,8312
	ctx.r11.s64 = ctx.r11.s64 + 8312;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r11,44
	ctx.r3.s64 = ctx.r11.s64 + 44;
	// b 0x82a3c440
	sub_82A3C440(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC0FE8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,72
	ctx.r5.s64 = 72;
	// addi r11,r11,8428
	ctx.r11.s64 = ctx.r11.s64 + 8428;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r11,44
	ctx.r3.s64 = ctx.r11.s64 + 44;
	// b 0x82a3c440
	sub_82A3C440(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1000) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r10,0
	ctx.r10.s64 = 0;
	// addi r11,r11,8544
	ctx.r11.s64 = ctx.r11.s64 + 8544;
	// stw r10,44(r11)
	REX_STORE_U32(ctx.r11.u32 + 44, ctx.r10.u32);
	// stw r10,48(r11)
	REX_STORE_U32(ctx.r11.u32 + 48, ctx.r10.u32);
	// stw r10,52(r11)
	REX_STORE_U32(ctx.r11.u32 + 52, ctx.r10.u32);
	// stw r10,56(r11)
	REX_STORE_U32(ctx.r11.u32 + 56, ctx.r10.u32);
	// stw r10,60(r11)
	REX_STORE_U32(ctx.r11.u32 + 60, ctx.r10.u32);
	// li r10,1
	ctx.r10.s64 = 1;
	// stw r10,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1030) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,72
	ctx.r5.s64 = 72;
	// addi r31,r11,8708
	ctx.r31.s64 = ctx.r11.s64 + 8708;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,44
	ctx.r3.s64 = ctx.r31.s64 + 44;
	// bl 0x82a3c440
	ctx.lr = 0x82CC1058;
	sub_82A3C440(ctx, base);
	// li r9,0
	ctx.r9.s64 = 0;
	// li r10,9
	ctx.r10.s64 = 9;
	// addi r11,r31,116
	ctx.r11.s64 = ctx.r31.s64 + 116;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_82CC1068:
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x82cc1068
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_82CC1068;
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1088) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// li r5,72
	ctx.r5.s64 = 72;
	// addi r31,r11,8860
	ctx.r31.s64 = ctx.r11.s64 + 8860;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r31,44
	ctx.r3.s64 = ctx.r31.s64 + 44;
	// bl 0x82a3c440
	ctx.lr = 0x82CC10B0;
	sub_82A3C440(ctx, base);
	// li r9,0
	ctx.r9.s64 = 0;
	// li r10,9
	ctx.r10.s64 = 9;
	// addi r11,r31,116
	ctx.r11.s64 = ctx.r31.s64 + 116;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_82CC10C0:
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x82cc10c0
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_82CC10C0;
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,152(r31)
	REX_STORE_U32(ctx.r31.u32 + 152, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC10E8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// li r5,160
	ctx.r5.s64 = 160;
	// addi r3,r11,5976
	ctx.r3.s64 = ctx.r11.s64 + 5976;
	// li r4,0
	ctx.r4.s64 = 0;
	// bl 0x82a3c440
	ctx.lr = 0x82CC1108;
	sub_82A3C440(ctx, base);
	// lis r11,-32052
	ctx.r11.s64 = -2100559872;
	// addi r3,r11,9360
	ctx.r3.s64 = ctx.r11.s64 + 9360;
	// bl 0x82a3cc28
	ctx.lr = 0x82CC1114;
	sub_82A3CC28(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1128) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,9020(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 9020);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// stfs f0,6776(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6776, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1140) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,9020(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 9020);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11484(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11484);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6772(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6772, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1160) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,9020(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 9020);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lfs f13,-11488(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -11488);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6784(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6784, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1180) {
	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// lfs f0,9020(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 9020);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,5108(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5108);
	ctx.f13.f64 = double(temp.f32);
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfs f0,6780(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 6780, temp.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC11A0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-192(r1)
	ea = -192 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// lwz r4,100(r1)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r1.u32 + 100);
	// li r10,1
	ctx.r10.s64 = 1;
	// addi r11,r11,-28524
	ctx.r11.s64 = ctx.r11.s64 + -28524;
	// addi r3,r1,92
	ctx.r3.s64 = ctx.r1.s64 + 92;
	// sth r10,86(r1)
	REX_STORE_U16(ctx.r1.u32 + 86, ctx.r10.u16);
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// bl 0x82ad5e28
	ctx.lr = 0x82CC11CC;
	sub_82AD5E28(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,92
	ctx.r3.s64 = ctx.r1.s64 + 92;
	// bl 0x82ad5e28
	ctx.lr = 0x82CC11D8;
	sub_82AD5E28(ctx, base);
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// lis r8,-32053
	ctx.r8.s64 = -2100625408;
	// addi r10,r11,-28488
	ctx.r10.s64 = ctx.r11.s64 + -28488;
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lis r9,-32053
	ctx.r9.s64 = -2100625408;
	// addi r11,r11,6968
	ctx.r11.s64 = ctx.r11.s64 + 6968;
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r10,r8,16896
	ctx.r10.s64 = ctx.r8.s64 + 16896;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// addi r10,r9,14584
	ctx.r10.s64 = ctx.r9.s64 + 14584;
	// stw r10,8(r11)
	REX_STORE_U32(ctx.r11.u32 + 8, ctx.r10.u32);
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// stw r10,12(r11)
	REX_STORE_U32(ctx.r11.u32 + 12, ctx.r10.u32);
	// addi r1,r1,192
	ctx.r1.s64 = ctx.r1.s64 + 192;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1220) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,26540
	ctx.r11.s64 = ctx.r11.s64 + 26540;
	// stw r11,6988(r10)
	REX_STORE_U32(ctx.r10.u32 + 6988, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1238) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r3,r11,27920
	ctx.r3.s64 = ctx.r11.s64 + 27920;
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// addi r11,r11,32144
	ctx.r11.s64 = ctx.r11.s64 + 32144;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8225d620
	sub_8225D620(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1250) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// lis r10,-32047
	ctx.r10.s64 = -2100232192;
	// addi r11,r11,30564
	ctx.r11.s64 = ctx.r11.s64 + 30564;
	// stw r11,-31004(r10)
	REX_STORE_U32(ctx.r10.u32 + -31004, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1268) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// lis r10,-32047
	ctx.r10.s64 = -2100232192;
	// addi r11,r11,30564
	ctx.r11.s64 = ctx.r11.s64 + 30564;
	// stw r11,-31000(r10)
	REX_STORE_U32(ctx.r10.u32 + -31000, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1280) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// lis r10,-32047
	ctx.r10.s64 = -2100232192;
	// addi r11,r11,30564
	ctx.r11.s64 = ctx.r11.s64 + 30564;
	// stw r11,-30996(r10)
	REX_STORE_U32(ctx.r10.u32 + -30996, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1298) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// lis r10,-32047
	ctx.r10.s64 = -2100232192;
	// addi r11,r11,30564
	ctx.r11.s64 = ctx.r11.s64 + 30564;
	// stw r11,-30992(r10)
	REX_STORE_U32(ctx.r10.u32 + -30992, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC12B0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,-15528
	ctx.r11.s64 = ctx.r11.s64 + -15528;
	// stw r11,27932(r10)
	REX_STORE_U32(ctx.r10.u32 + 27932, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC12C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,-15528
	ctx.r11.s64 = ctx.r11.s64 + -15528;
	// stw r11,27952(r10)
	REX_STORE_U32(ctx.r10.u32 + 27952, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC12E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r3,r11,27976
	ctx.r3.s64 = ctx.r11.s64 + 27976;
	// b 0x8285ab90
	sub_8285AB90(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC12F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// li r4,1
	ctx.r4.s64 = 1;
	// addi r3,r11,29288
	ctx.r3.s64 = ctx.r11.s64 + 29288;
	// b 0x82329f48
	sub_82329F48(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1300) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// lis r10,-32047
	ctx.r10.s64 = -2100232192;
	// addi r11,r11,-13644
	ctx.r11.s64 = ctx.r11.s64 + -13644;
	// stw r11,-30984(r10)
	REX_STORE_U32(ctx.r10.u32 + -30984, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1318) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1320) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1328) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1330) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1338) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32047
	ctx.r11.s64 = -2100232192;
	// addi r11,r11,-27100
	ctx.r11.s64 = ctx.r11.s64 + -27100;
	// lwz r4,0(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// cmplwi cr6,r4,0
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// lhz r5,6(r11)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r3,r10,-10032
	ctx.r3.s64 = ctx.r10.s64 + -10032;
	// b 0x82742f18
	sub_82742F18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC135C) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1360) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32047
	ctx.r11.s64 = -2100232192;
	// addi r11,r11,-27084
	ctx.r11.s64 = ctx.r11.s64 + -27084;
	// lwz r4,0(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// cmplwi cr6,r4,0
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// lhz r5,6(r11)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r3,r10,-10032
	ctx.r3.s64 = ctx.r10.s64 + -10032;
	// b 0x82742f18
	sub_82742F18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1384) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1388) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1390) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1398) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13A0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13A8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13B0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13B8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13C0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13C8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13D0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13D8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13E0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13E8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13F0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC13F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1400) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1408) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1410) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1418) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1420) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1428) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1430) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lis r10,-32051
	ctx.r10.s64 = -2100494336;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,5040(r10)
	REX_STORE_U32(ctx.r10.u32 + 5040, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1448) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1450) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1458) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1460) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1468) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32226
	ctx.r11.s64 = -2111963136;
	// addi r10,r11,-15264
	ctx.r10.s64 = ctx.r11.s64 + -15264;
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r9,r11,15200
	ctx.r9.s64 = ctx.r11.s64 + 15200;
	// lis r11,-32043
	ctx.r11.s64 = -2099970048;
	// addi r11,r11,29260
	ctx.r11.s64 = ctx.r11.s64 + 29260;
	// stw r10,4(r11)
	REX_STORE_U32(ctx.r11.u32 + 4, ctx.r10.u32);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1490) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1498) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14A0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14A8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14B0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r3,r11,-10472
	ctx.r3.s64 = ctx.r11.s64 + -10472;
	// lis r11,-32245
	ctx.r11.s64 = -2113208320;
	// addi r11,r11,24892
	ctx.r11.s64 = ctx.r11.s64 + 24892;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8270ccf8
	sub_8270CCF8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC14D0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r3,r11,-10452
	ctx.r3.s64 = ctx.r11.s64 + -10452;
	// lis r11,-32245
	ctx.r11.s64 = -2113208320;
	// addi r11,r11,24892
	ctx.r11.s64 = ctx.r11.s64 + 24892;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8270ccf8
	sub_8270CCF8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC14E8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14F0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC14F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1500) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1508) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1510) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1518) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1520) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1528) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1530) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1538) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1540) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1548) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1550) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1558) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32043
	ctx.r11.s64 = -2099970048;
	// addi r11,r11,30320
	ctx.r11.s64 = ctx.r11.s64 + 30320;
	// lwz r4,0(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// cmplwi cr6,r4,0
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// lis r10,-32032
	ctx.r10.s64 = -2099249152;
	// lhz r5,6(r11)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r3,r10,-10032
	ctx.r3.s64 = ctx.r10.s64 + -10032;
	// b 0x82742f18
	sub_82742F18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC157C) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1580) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1588) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1590) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1598) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15A0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15A8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15B0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// addi r3,r11,-9592
	ctx.r3.s64 = ctx.r11.s64 + -9592;
	// b 0x8271f600
	sub_8271F600(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC15C8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15D0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15D8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15E0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15E8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15F0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC15F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1600) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1608) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1610) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// addi r3,r11,7528
	ctx.r3.s64 = ctx.r11.s64 + 7528;
	// b 0x823296d8
	sub_823296D8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1620) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1628) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1630) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1638) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1640) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1648) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1650) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1658) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1660) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1668) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1670) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1678) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1680) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1688) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1690) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1698) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16A0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16A8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16B0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16B8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16C0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16C8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16D0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16D8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lis r10,-32042
	ctx.r10.s64 = -2099904512;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,12020(r10)
	REX_STORE_U32(ctx.r10.u32 + 12020, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC16F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lis r10,-32042
	ctx.r10.s64 = -2099904512;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,12028(r10)
	REX_STORE_U32(ctx.r10.u32 + 12028, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1710) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// lwz r3,12356(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 12356);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1720) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// lwz r3,12380(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 12380);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1730) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32042
	ctx.r11.s64 = -2099904512;
	// addi r3,r11,12368
	ctx.r3.s64 = ctx.r11.s64 + 12368;
	// b 0x82700690
	sub_82700690(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1740) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1748) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1750) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lwz r3,-16244(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + -16244);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1760) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1768) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1770) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r3,r11,964
	ctx.r3.s64 = ctx.r11.s64 + 964;
	// b 0x82715668
	sub_82715668(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1780) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// addi r3,r1,88
	ctx.r3.s64 = ctx.r1.s64 + 88;
	// addi r31,r11,-13556
	ctx.r31.s64 = ctx.r11.s64 + -13556;
	// mr r4,r31
	ctx.r4.u64 = ctx.r31.u64;
	// lwz r11,4(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 4);
	// HAND PATCH: this is a static-object destructor called from the
	// C-runtime destructor walker; the member pointer at r31+4 can be null
	// (object never constructed on this boot path, or torn down already --
	// this very function zeroes the field at its end). Nothing to clean up;
	// take the function's own epilogue early instead of dereferencing null.
	if (ctx.r11.u32 == 0) {
		ctx.r1.s64 = ctx.r1.s64 + 112;
		ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
		ctx.lr = ctx.r12.u64;
		ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
		return;
	}
	// stw r31,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r31.u32);
	// stw r31,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r31.u32);
	// lwz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// ld r6,80(r1)
	ctx.r6.u64 = REX_LOAD_U64(ctx.r1.u32 + 80);
	// stw r10,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r10.u32);
	// ld r5,88(r1)
	ctx.r5.u64 = REX_LOAD_U64(ctx.r1.u32 + 88);
	// bl 0x82717798
	ctx.lr = 0x82CC17C4;
	sub_82717798(ctx, base);
	// lwz r3,4(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 4);
	// bl 0x8269beb0
	ctx.lr = 0x82CC17CC;
	sub_8269BEB0(ctx, base);
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,4(r31)
	REX_STORE_U32(ctx.r31.u32 + 4, ctx.r11.u32);
	// stw r11,8(r31)
	REX_STORE_U32(ctx.r31.u32 + 8, ctx.r11.u32);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC17F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// lwz r3,-13168(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + -13168);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1800) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1808) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1810) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1818) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1820) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r31,-32041
	ctx.r31.s64 = -2099838976;
	// lwz r3,-11528(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + -11528);
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beq cr6,0x82cc184c
	if (ctx.cr6.eq) goto loc_82CC184C;
	// bl 0x8269beb0
	ctx.lr = 0x82CC1844;
	sub_8269BEB0(ctx, base);
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,-11528(r31)
	REX_STORE_U32(ctx.r31.u32 + -11528, ctx.r11.u32);
loc_82CC184C:
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1860) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32041
	ctx.r11.s64 = -2099838976;
	// addi r3,r11,-10156
	ctx.r3.s64 = ctx.r11.s64 + -10156;
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// addi r11,r11,-8
	ctx.r11.s64 = ctx.r11.s64 + -8;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x82690790
	sub_82690790(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1878) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1880) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r11,r11,-10052
	ctx.r11.s64 = ctx.r11.s64 + -10052;
	// lwz r3,0(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// lhz r4,6(r11)
	ctx.r4.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// b 0x8238f190
	sub_8238F190(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC189C) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC18A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,-10040
	ctx.r3.s64 = ctx.r11.s64 + -10040;
	// b 0x8274a838
	sub_8274A838(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC18B0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,-10032
	ctx.r3.s64 = ctx.r11.s64 + -10032;
	// b 0x827430d8
	sub_827430D8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC18C0) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r31,r11,8008
	ctx.r31.s64 = ctx.r11.s64 + 8008;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274e250
	ctx.lr = 0x82CC18E0;
	sub_8274E250(ctx, base);
	// lwz r3,0(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// bl 0x8269bf10
	ctx.lr = 0x82CC18E8;
	sub_8269BF10(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1900) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r31,r11,8032
	ctx.r31.s64 = ctx.r11.s64 + 8032;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8274f7a8
	ctx.lr = 0x82CC1920;
	sub_8274F7A8(ctx, base);
	// lwz r3,0(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// bl 0x8269bf10
	ctx.lr = 0x82CC1928;
	sub_8269BF10(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1940) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// addi r10,r11,5920
	ctx.r10.s64 = ctx.r11.s64 + 5920;
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r11,r11,8060
	ctx.r11.s64 = ctx.r11.s64 + 8060;
	// lwz r3,4(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1964) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1968) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1970) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9092(r10)
	REX_STORE_U32(ctx.r10.u32 + 9092, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1988) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9268(r10)
	REX_STORE_U32(ctx.r10.u32 + 9268, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC19A0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r11,r11,-3288
	ctx.r11.s64 = ctx.r11.s64 + -3288;
	// lwz r3,4(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC19B0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9780(r10)
	REX_STORE_U32(ctx.r10.u32 + 9780, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC19C8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9828(r10)
	REX_STORE_U32(ctx.r10.u32 + 9828, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC19E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9876(r10)
	REX_STORE_U32(ctx.r10.u32 + 9876, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC19F8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9924(r10)
	REX_STORE_U32(ctx.r10.u32 + 9924, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A10) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,9972(r10)
	REX_STORE_U32(ctx.r10.u32 + 9972, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A28) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10020(r10)
	REX_STORE_U32(ctx.r10.u32 + 10020, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A40) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10068(r10)
	REX_STORE_U32(ctx.r10.u32 + 10068, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A58) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10116(r10)
	REX_STORE_U32(ctx.r10.u32 + 10116, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A70) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10164(r10)
	REX_STORE_U32(ctx.r10.u32 + 10164, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1A88) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10212(r10)
	REX_STORE_U32(ctx.r10.u32 + 10212, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1AA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10260(r10)
	REX_STORE_U32(ctx.r10.u32 + 10260, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1AB8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10308(r10)
	REX_STORE_U32(ctx.r10.u32 + 10308, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1AD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10356(r10)
	REX_STORE_U32(ctx.r10.u32 + 10356, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1AE8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10404(r10)
	REX_STORE_U32(ctx.r10.u32 + 10404, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B00) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10456(r10)
	REX_STORE_U32(ctx.r10.u32 + 10456, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B18) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32049
	ctx.r10.s64 = -2100363264;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,10496(r10)
	REX_STORE_U32(ctx.r10.u32 + 10496, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B30) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,6896
	ctx.r3.s64 = ctx.r11.s64 + 6896;
	// b 0x823296d8
	sub_823296D8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1B48) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B50) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B58) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B60) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1B68) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r3,r11,24560
	ctx.r3.s64 = ctx.r11.s64 + 24560;
	// b 0x827c7bb8
	sub_827C7BB8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1B78) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r30,-24(r1)
	REX_STORE_U64(ctx.r1.u32 + -24, ctx.r30.u64);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// li r30,177
	ctx.r30.s64 = 177;
	// addi r11,r11,9592
	ctx.r11.s64 = ctx.r11.s64 + 9592;
	// addi r31,r11,712
	ctx.r31.s64 = ctx.r11.s64 + 712;
loc_82CC1B9C:
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x827c7bb8
	ctx.lr = 0x82CC1BA8;
	sub_827C7BB8(ctx, base);
	// addi r30,r30,-1
	ctx.r30.s64 = ctx.r30.s64 + -1;
	// cmpwi cr6,r30,0
	ctx.cr6.compare<int32_t>(ctx.r30.s32, 0, ctx.xer);
	// bge cr6,0x82cc1b9c
	if (!ctx.cr6.lt) goto loc_82CC1B9C;
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r30,-24(r1)
	ctx.r30.u64 = REX_LOAD_U64(ctx.r1.u32 + -24);
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1BD0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,10304
	ctx.r3.s64 = ctx.r11.s64 + 10304;
	// b 0x827d4280
	sub_827D4280(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1BE0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32049
	ctx.r11.s64 = -2100363264;
	// addi r3,r11,29496
	ctx.r3.s64 = ctx.r11.s64 + 29496;
	// b 0x827c7bb8
	sub_827C7BB8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1BF0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1BF8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C00) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C08) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C10) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C18) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C20) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C28) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C30) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C38) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C40) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C48) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C50) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C58) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C60) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,-31348(r10)
	REX_STORE_U32(ctx.r10.u32 + -31348, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C78) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C80) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1C88) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,-30972(r10)
	REX_STORE_U32(ctx.r10.u32 + -30972, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CA0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r4,r11,30328
	ctx.r4.s64 = ctx.r11.s64 + 30328;
	// lwz r3,0(r4)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// b 0x827455e0
	sub_827455E0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1CB8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CC0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CC8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CD0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CD8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CE0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CE8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CF0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1CF8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D00) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D08) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D10) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D18) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D20) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D28) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D30) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D38) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D40) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D48) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D50) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D58) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D60) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D68) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D70) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D78) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D80) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D88) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D90) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1D98) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DA0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DA8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DB0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DB8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DC0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DC8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DD0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DD8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DE0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DE8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DF0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1DF8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E00) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E08) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E10) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E18) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E20) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E28) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E30) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E38) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,31208
	ctx.r3.s64 = ctx.r11.s64 + 31208;
	// b 0x823296d8
	sub_823296D8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1E48) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E50) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E58) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E60) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,31300
	ctx.r3.s64 = ctx.r11.s64 + 31300;
	// lis r11,-32232
	ctx.r11.s64 = -2112356352;
	// addi r11,r11,19788
	ctx.r11.s64 = ctx.r11.s64 + 19788;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8274ab10
	sub_8274AB10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC1E78) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E80) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E88) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E90) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1E98) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EA0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EA8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EB0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EB8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EC0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EC8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1ED0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1ED8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EE0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EE8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EF0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1EF8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F00) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F08) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F10) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F18) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F20) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F28) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F30) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F38) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F40) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F48) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F50) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F58) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F60) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F68) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F70) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F78) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F80) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F88) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F90) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1F98) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FA0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FA8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FB0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FB8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FC0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FC8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FD0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FD8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FE0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FE8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FF0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC1FF8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2000) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2008) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2010) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2018) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2020) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2028) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2030) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2038) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2040) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2048) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2050) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2058) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2060) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2068) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2070) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2078) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2080) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2088) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2090) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2098) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20A0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// lwz r3,32080(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 32080);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC20B8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// lwz r3,32052(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 32052);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC20C8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20D0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20D8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20E0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20E8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20F0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC20F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2100) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32032
	ctx.r11.s64 = -2099249152;
	// addi r3,r11,32324
	ctx.r3.s64 = ctx.r11.s64 + 32324;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// addi r11,r11,-18768
	ctx.r11.s64 = ctx.r11.s64 + -18768;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8274ab10
	sub_8274AB10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC2118) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2120) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2128) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2130) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r31,r11,-28528
	ctx.r31.s64 = ctx.r11.s64 + -28528;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270cb00
	ctx.lr = 0x82CC2150;
	sub_8270CB00(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2170) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r31,r11,-28488
	ctx.r31.s64 = ctx.r11.s64 + -28488;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270cb00
	ctx.lr = 0x82CC2190;
	sub_8270CB00(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21B0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21B8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21C0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21C8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21D0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21D8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21E0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21E8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21F0) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC21F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2200) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2208) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2210) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2218) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2220) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2228) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2230) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2238) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2240) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2248) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2250) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2258) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2260) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2268) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2270) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2278) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r31,r11,-27380
	ctx.r31.s64 = ctx.r11.s64 + -27380;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270cb00
	ctx.lr = 0x82CC2298;
	sub_8270CB00(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC22B8) {
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r11,-32048
	ctx.r11.s64 = -2100297728;
	// addi r31,r11,-27340
	ctx.r31.s64 = ctx.r11.s64 + -27340;
	// addi r3,r31,4
	ctx.r3.s64 = ctx.r31.s64 + 4;
	// bl 0x8270cb00
	ctx.lr = 0x82CC22D8;
	sub_8270CB00(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r11,r11,15200
	ctx.r11.s64 = ctx.r11.s64 + 15200;
	// stw r11,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r11.u32);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC22F8) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2300) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2308) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2310) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2318) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2320) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// lwz r3,27276(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 27276);
	// b 0x8269bf10
	sub_8269BF10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC2330) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2338) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2340) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2348) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2350) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2358) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2360) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32235
	ctx.r11.s64 = -2112552960;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,12168
	ctx.r11.s64 = ctx.r11.s64 + 12168;
	// stw r11,-24608(r10)
	REX_STORE_U32(ctx.r10.u32 + -24608, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2378) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32030
	ctx.r11.s64 = -2099118080;
	// addi r3,r11,27736
	ctx.r3.s64 = ctx.r11.s64 + 27736;
	// lis r11,-32231
	ctx.r11.s64 = -2112290816;
	// addi r11,r11,25860
	ctx.r11.s64 = ctx.r11.s64 + 25860;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// b 0x8274ab10
	sub_8274AB10(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC2390) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,-25716
	ctx.r11.s64 = ctx.r11.s64 + -25716;
	// stw r11,-13320(r10)
	REX_STORE_U32(ctx.r10.u32 + -13320, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC23A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,-25716
	ctx.r11.s64 = ctx.r11.s64 + -25716;
	// stw r11,-11592(r10)
	REX_STORE_U32(ctx.r10.u32 + -11592, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC23C0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lwz r3,-26624(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + -26624);
	// b 0x82cc2f24
	__imp__KeTlsFree(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC23D0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lwz r3,-26120(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + -26120);
	// b 0x82cc2f24
	__imp__KeTlsFree(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC23E0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// addi r3,r11,-26108
	ctx.r3.s64 = ctx.r11.s64 + -26108;
	// b 0x82b6c930
	sub_82B6C930(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC23F0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32029
	ctx.r11.s64 = -2099052544;
	// lwz r3,-26112(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + -26112);
	// b 0x82cc2f24
	__imp__KeTlsFree(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82CC2400) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2408) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32230
	ctx.r11.s64 = -2112225280;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,10420
	ctx.r11.s64 = ctx.r11.s64 + 10420;
	// stw r11,-5888(r10)
	REX_STORE_U32(ctx.r10.u32 + -5888, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2420) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,10136
	ctx.r11.s64 = ctx.r11.s64 + 10136;
	// stw r11,5184(r10)
	REX_STORE_U32(ctx.r10.u32 + 5184, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2438) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,10136
	ctx.r11.s64 = ctx.r11.s64 + 10136;
	// stw r11,5216(r10)
	REX_STORE_U32(ctx.r10.u32 + 5216, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2450) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2458) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2460) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2468) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2470) {
	REX_FUNC_PROLOGUE();
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2478) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,27800
	ctx.r11.s64 = ctx.r11.s64 + 27800;
	// stw r11,6136(r10)
	REX_STORE_U32(ctx.r10.u32 + 6136, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC2490) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,27788
	ctx.r11.s64 = ctx.r11.s64 + 27788;
	// stw r11,5976(r10)
	REX_STORE_U32(ctx.r10.u32 + 5976, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC24A8) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32029
	ctx.r10.s64 = -2099052544;
	// addi r11,r11,27788
	ctx.r11.s64 = ctx.r11.s64 + 27788;
	// stw r11,6436(r10)
	REX_STORE_U32(ctx.r10.u32 + 6436, ctx.r11.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82CC24C0) {
	REX_FUNC_PROLOGUE();
	// lis r11,-32229
	ctx.r11.s64 = -2112159744;
	// lis r10,-32048
	ctx.r10.s64 = -2100297728;
	// addi r11,r11,27788
	ctx.r11.s64 = ctx.r11.s64 + 27788;
	// stw r11,5556(r10)
	REX_STORE_U32(ctx.r10.u32 + 5556, ctx.r11.u32);
	// blr 
	return;
}

