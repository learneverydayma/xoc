/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#include "cominc.h"
#include "prdf.h"
#include "dex.h"
#include "gra.h"

static CHAR const* g_fmt_name[] = {
    "",
    "F0",
    "FAB",
    "FABcv",
    "FAA",
    "FAAv",
    "FAAAAv",
    "FAABBBB",
    "FAABBBBv",
    "FAABBBBcv",
    "FAABBBBcvh",
    "FAABBCC",
    "FAABBCCcv",
    "FABCCCCv",
    "FABCCCCcv",
    "FAAAABBBB",
    "FAAAAAAAAv",
    "FAABBBBBBBBv",
    "FAABBBBBBBBcv",
    "FACDEFGBBBBv",
    "FAACCCCBBBBv",
    "FAABBBBBBBBBBBBBBBBcv",
};

#define PAIR_BYTES 8

class OP_MATCH {
public:
    bool match_bin_r_rr(IR_TYPE irt, IR * ir, IR ** res, IR ** op0, IR ** op1)
    {
        if (!ir->is_stpr()) return false;
        if (res != NULL) { *res = ir; }

        IR * rhs = ST_rhs(ir);
        if ((UINT)IR_code(rhs) != (UINT)irt) return false;

        if (!BIN_opnd0(rhs)->is_pr()) return false;
        if (op0 != NULL) { *op0 = BIN_opnd0(rhs); }

        if (!BIN_opnd1(rhs)->is_pr()) return false;
        if (op1 != NULL) { *op1 = BIN_opnd1(rhs); }
        return true;
    }

    bool match_bin_r_rr(IR * ir, IR ** res, IR ** op0, IR ** op1)
    {
        if (!ir->is_stpr()) return false;
        if (res != NULL) { *res = ir; }

        switch (ST_rhs(ir)->get_code()) {
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_REM:
        case IR_MOD:
        case IR_LAND:
        case IR_LOR:
        case IR_BAND:
        case IR_BOR:
        case IR_XOR:
        case IR_ASR:
        case IR_LSR:
        case IR_LSL:
        case IR_LT:
        case IR_LE:
        case IR_GT:
        case IR_GE:
        case IR_EQ:
        case IR_NE:
            break;
        default: return false;
        }
        if (!BIN_opnd0(ST_rhs(ir))->is_pr()) return false;
        if (op0 != NULL) { *op0 = BIN_opnd0(ST_rhs(ir)); }
        if (!BIN_opnd1(ST_rhs(ir))->is_pr()) return false;
        if (op1 != NULL) { *op1 = BIN_opnd1(ST_rhs(ir)); }
        return true;
    }

    bool match_uni_r_r(IR_TYPE irt, IR * ir, IR ** res, IR ** op0)
    {
        if (!ir->is_stpr()) { return false; }
        if (res != NULL) { *res = ir; }
        if ((UINT)IR_code(ST_rhs(ir)) != (UINT)irt) return false;
        if (!UNA_opnd0(ST_rhs(ir))->is_pr()) return false;
        if (op0 != NULL) { *op0 = UNA_opnd0(ST_rhs(ir)); }
        return true;
    }

    bool match_uni_r_r(IR * ir, IR ** res, IR ** op0)
    {
        if (!ir->is_stpr()) return false;
        IR * rhs = ST_rhs(ir);
        switch (IR_code(rhs)) {
        case IR_BNOT:
        case IR_LNOT:
        case IR_NEG:
            break;
        default: return false;
        }
        if (res != NULL) { *res = ir; }
        if (!UNA_opnd0(rhs)->is_pr()) return false;
        if (op0 != NULL) { *op0 = UNA_opnd0(rhs); }
        return true;
    }

    bool match_mv_r_r(IR * ir, IR ** res, IR ** op0)
    {
        if (!ir->is_stpr()) return false;
        if (res != NULL) { *res = ir; }
        if (!ST_rhs(ir)->is_pr()) return false;
        if (op0 != NULL) { *op0 = ST_rhs(ir); }
        return true;
    }
};


//
//START GLT
//
UINT GLT::computeNumOfOcc(GltMgr & gltm)
{
    if (livebbs == NULL) { return 0; }
    SEGIter * cur = NULL;
    UINT n = 0;
    for (INT i = livebbs->get_first(&cur);
         i >= 0; i = livebbs->get_next(i, &cur)) {
        LTMgr * ltm = gltm.get_ltm(i);
        ASSERT0(ltm);
        LT * l = ltm->map_pr2lt(prno);
        ASSERT0(l);
        if (LT_occ(l) == NULL) { continue; }
        n += LT_occ(l)->get_elem_count();
    }
    return n;
}


//Set usable for local part.
void GLT::set_local_usable(GltMgr & gltm)
{
    SEGIter * sc = NULL;
    for (INT j = livebbs->get_first(&sc);
         j >= 0; j = livebbs->get_next(j, &sc)) {
        LTMgr * ltm = gltm.get_ltm(j);
        if (ltm == NULL) { continue; }
        LT * l = ltm->map_pr2lt(prno);
        ASSERT0(l); //glt miss local part
        LT_usable(l) = usable;
    }
}


//Set local part has same info with the global lt.
void GLT::set_local(GltMgr & gltm)
{
    if (livebbs == NULL) { return; }
    SEGIter * cur = NULL;
    for (INT i = livebbs->get_first(&cur);
         i >= 0; i = livebbs->get_next(i, &cur)) {
        LTMgr * ltm = gltm.get_ltm(i);
        ASSERT(ltm, ("miss local part"));
        LT * l = ltm->map_pr2lt(prno);
        ASSERT(l, ("miss local part"));
        LT_phy(l) = phy;
        LT_rg_sz(l) = reg_group_size;
        LT_usable(l) = usable;
    }
}
//END GLT


//
//START LT
//
//Return global lt.
void LT::clean()
{
    range->clean();
    if (occ != NULL) {
        occ->clean();
    }
    return; //Do not clean other info.

    /*
    priority = 0.0;
    //usable = NULL;
    phy = REG_UNDEF;
    prefer_reg = REG_UNDEF;
    reg_group_size = 1;
    lt_group_pos = 0;
    if (lt_group != NULL) {
        lt_group->clean();
    }
    */
}


GLT * LT::set_global(GltMgr & gltm)
{
    GLT * g = gltm.map_pr2glt(prno);
    ASSERT0(g);
    GLT_phy(g) = phy;
    return g;
}


//Return true if lt has occurred in a branch.
bool LT::has_branch(LTMgr * ltm) const
{
    if (occ == NULL) { return false; }
    INT i = occ->get_last();
    if (i < 0) { return false; }
    IR * occ = ltm->get_ir(i);
    if (occ->is_cond_br() || occ->is_uncond_br() ||
        occ->is_multicond_br()) {
        return true;
    }
    return false;
}
//END LT


//
//START RSC
//
BitSet * RSC::get_4()
{
    if (m_4 == NULL) {
        m_4 = m_bsm->create(2);
        ASSERT0(FIRST_PHY_REG == 0);
        for (INT i = 0; i <= 15; i++) {
            m_4->bunion(i);
        }
    }
    return m_4;
}


BitSet * RSC::get_8()
{
    if (m_8 == NULL) {
        m_8 = m_bsm->create(8);
        ASSERT0(FIRST_PHY_REG == 0);
        for (INT i = 0; i <= 63; i++) {
            m_8->bunion(i);
        }
    }
    return m_8;
}


BitSet * RSC::get_16()
{
    if (m_16 == NULL) {
        m_16 = m_bsm->create(128);
        ASSERT0(FIRST_PHY_REG == 0);
        for (INT i = 0; i <= 1023; i++) {
            m_16->bunion(i);
        }
    }
    return m_16;
}


void RSC::comp_st_fmt(IR const* ir)
{
    ASSERT0(ir->is_st() || ir->is_stpr());
    IR const* stv = ir->get_rhs();
    comp_ir_fmt(stv);
    if (ir->is_stpr()) {
        switch (stv->get_code()) {
        case IR_CONST:
            //A, +B, load const
            m_ir2fmt.set(IR_id(ir), FABcv);
            break;
        case IR_LD:
            //AABBBB, sget
            m_ir2fmt.set(IR_id(ir), FAABBBB);
            break;
        case IR_ILD:
            //ABCCCC, iget
            m_ir2fmt.set(IR_id(ir), FABCCCCv);
            return;
        case IR_LDA:
            //AABBBB
            m_ir2fmt.set(IR_id(ir), FAABBBBv);
            return;
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_REM:
        case IR_MOD:
        case IR_BAND:
        case IR_BOR:
        case IR_XOR:
        case IR_ASR:
        case IR_LSR:
        case IR_LSL:
            {
                IR * op1 = BIN_opnd1(stv);
                ASSERT0(BIN_opnd0(stv)->is_pr());
                if (op1->is_const()) {
                    ASSERT0(op1->is_int());
                    if (!is_s8((INT)CONST_int_val(op1))) {
                        //vA, vB, CCCC+
                        m_ir2fmt.set(IR_id(ir), FABCCCCv);
                    } else {
                        ASSERT0(is_s16((INT)CONST_int_val(op1)));
                        //vAA, vAB, CC+
                        m_ir2fmt.set(IR_id(ir), FAABBCCcv);
                    }
                } else {
                    //AABBCC, v0 = v1 op v2
                    m_ir2fmt.set(IR_id(ir), FAABBCC);
                }
            }
            return;
        case IR_LT:
        case IR_LE:
        case IR_GT:
        case IR_GE:
        case IR_EQ:
        case IR_NE:
            //AABBCC, v0 = v1 op v2
            m_ir2fmt.set(IR_id(ir), FAABBCC);
            return;
        case IR_ARRAY:
            //AABBCC.
            //aget, OBJ, v0 <- (base)v1, (ofst)v2
            ASSERT0(ARR_base(stv)->is_pr() &&
                     ARR_sub_list(stv)->is_pr() &&
                     ((CArray*)stv)->getDimNum() == 1);
            m_ir2fmt.set(IR_id(ir), FAABBCC);
            return;
        case IR_BNOT:
        case IR_LNOT:
        case IR_NEG:
            //AB.
            ASSERT(UNA_opnd0(stv)->is_pr(), ("unary op base must be pr"));
            m_ir2fmt.set(IR_id(ir), FAB);
            return;
        case IR_CVT:
            //AB.
            ASSERT(CVT_exp(stv)->is_pr(), ("cvt base must be pr"));
            m_ir2fmt.set(IR_id(ir), FAB);
            return;
        case IR_PR:
            //AAAABBBB
            m_ir2fmt.set(IR_id(ir), FAAAABBBB);
            return;
        default: UNREACH();
        }
    } else {
        ASSERT0(ir->is_st());
        switch (stv->get_code()) {
        case IR_PR:
            //AABBBB, sput
            m_ir2fmt.set(IR_id(ir), FAABBBB);
            break;
        case IR_ILD:
            //ABCCCC, iget
            m_ir2fmt.set(IR_id(ir), FABCCCCv);
            return;
        default: UNREACH();
        }
    }
}


void RSC::comp_starray_fmt(IR const* ir)
{
    ASSERT0(ir->is_starray());
    IR const* stv = STARR_rhs(ir);
    comp_ir_fmt(stv);
    ASSERT0(stv->is_pr());

    ASSERT(((CArray*)ir)->getDimNum() == 1, ("dex supply only one dim array"));

    //AABBCC
    //aput, OBJ, vAA -> (array_base_ptr)vBB, (array_elem)vCC
    ASSERT0(ARR_base(ir)->is_pr() && ARR_sub_list(ir)->is_pr());

    m_ir2fmt.set(IR_id(ir), FAABBCC);
}


void RSC::comp_ist_fmt(IR const* ir)
{
    ASSERT0(ir->is_ist());
    IR const* stv = IST_rhs(ir);
    comp_ir_fmt(stv);
    ASSERT0(stv->is_pr());

    IR const* lhs = IST_base(ir);
    ASSERT0(lhs->is_pr());
    UNUSED(lhs);
    //ABCCCC
    //iput, vA(stv) -> vB(mlr), +CCCC(field_id)
    m_ir2fmt.set(IR_id(ir), FABCCCCv);
}


void RSC::comp_call_fmt(IR const* ir)
{
    ASSERT0(ir->is_calls_stmt());
    if (ir->is_icall()) {
        comp_ir_fmt(ICALL_callee(ir));
    }

    if (ir->is_call() && CALL_is_intrinsic(ir)) {
        VAR const* v = CALL_idinfo(ir);
        ASSERT0(v);
        BLTIN_TYPE blt = m_str2builtin.get(SYM_name(v->get_name()));
        ASSERT0(blt != BLTIN_UNDEF);
        switch (blt) {
        case BLTIN_NEW:
            m_ir2fmt.set(IR_id(ir), FAABBBBv);
            return;
        case BLTIN_NEW_ARRAY:
            m_ir2fmt.set(IR_id(ir), FABCCCCv);
            return;
        case BLTIN_MOVE_EXP:
        case BLTIN_MOVE_RES:
            m_ir2fmt.set(IR_id(ir), FAA);
            return;
        case BLTIN_THROW:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAA);
            return;
        case BLTIN_CHECK_CAST:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAABBBBv);
            return;
        case BLTIN_FILLED_NEW_ARRAY:
            {
            //first parameter is invoke-kind.
            IR * p = CALL_param_list(ir);
            ASSERT0(p->is_const());

            //second one is class-id.
            p = p->get_next();
            ASSERT0(p->is_const());

            p = p->get_next();
            for (; p != NULL; p = p->get_next()) {
                ASSERT0(p->is_pr());
            }
            m_ir2fmt.set(IR_id(ir), FACDEFGBBBBv);
            return;
            }
        case BLTIN_FILL_ARRAY_DATA:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAABBBBBBBBv);
            return;
        case BLTIN_CONST_CLASS:
            m_ir2fmt.set(IR_id(ir), FAABBBBv);
            return;
        case BLTIN_ARRAY_LENGTH:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAB);
            return;
        case BLTIN_MONITOR_ENTER:
        case BLTIN_MONITOR_EXIT:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAA);
            return;
        case BLTIN_INSTANCE_OF:
            ASSERT0(CALL_param_list(ir) && CALL_param_list(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FABCCCCv);
            return;
        case BLTIN_CMP_BIAS:
            {
                IR const* p = CALL_param_list(ir);
                ASSERT0(p && p->is_const());
                p = p->get_next();
                ASSERT0(p && p->is_pr() && p->get_next() && p->get_next()->is_pr());
                m_ir2fmt.set(IR_id(ir), FAABBCC);
                return;
            }
        default:ASSERT(0, ("Unknown intrinsic"));
        }
    }

    IR const* p = CALL_param_list(ir);
    ASSERT0(p && p->is_const());
    INVOKE_KIND ik = (INVOKE_KIND)CONST_int_val(p);
    p = p->get_next();
    ASSERT0(p && p->is_const());
    p = p->get_next();
    switch (ik) {
    case INVOKE_UNDEF:
    case INVOKE_VIRTUAL:
    case INVOKE_SUPER:
    case INVOKE_DIRECT:
    case INVOKE_STATIC:
    case INVOKE_INTERFACE:
        {
            ASSERT0(cnt_list(p) <= 5);
            m_ir2fmt.set(IR_id(ir), FACDEFGBBBBv);
        }
        break;
    case INVOKE_VIRTUAL_RANGE:
    case INVOKE_SUPER_RANGE:
    case INVOKE_DIRECT_RANGE:
    case INVOKE_STATIC_RANGE:
    case INVOKE_INTERFACE_RANGE:
        m_ir2fmt.set(IR_id(ir), FAACCCCBBBBv);
        break;
    default: UNREACH();
    }

    for (; p != NULL; p = p->get_next()) {
        ASSERT0(p->is_pr());
    }
}


void RSC::comp_ir_fmt(IR const* ir)
{
    switch (IR_code(ir)) {
    case IR_CONST:
    case IR_ID:
    case IR_LD:
        return;
    case IR_ST:
    case IR_STPR:
        comp_st_fmt(ir);
        return;
    case IR_ILD:
        //ABCCCC
        //v%d(res) <- v%d(op0), field_id(op1)
        comp_ir_fmt(ILD_base(ir));
        if (ILD_base(ir)->is_pr()) {
            m_ir2fmt.set(IR_id(ir), FABCCCCv);
        } else { UNREACH(); }
        return;
    case IR_STARRAY:
        comp_starray_fmt(ir);
        return;
    case IR_IST:
        comp_ist_fmt(ir);
        return;
    case IR_LDA:
        return;
    case IR_CALL:
    case IR_ICALL:
        comp_call_fmt(ir);
        return;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_REM:
    case IR_MOD:
    case IR_LAND:
    case IR_LOR:
    case IR_BAND:
    case IR_BOR:
    case IR_XOR:
    case IR_LT:
    case IR_LE:
    case IR_GT:
    case IR_GE:
    case IR_EQ:
    case IR_NE:
    case IR_ASR:
    case IR_LSR:
    case IR_LSL:
        comp_ir_fmt(BIN_opnd0(ir));
        comp_ir_fmt(BIN_opnd1(ir));
        return;
    case IR_BNOT:
    case IR_LNOT:
    case IR_NEG:
        comp_ir_fmt(UNA_opnd0(ir));
        return;
    case IR_GOTO:
    case IR_LABEL:
        m_ir2fmt.set(IR_id(ir), F0);
        return;
    case IR_SWITCH:
        comp_ir_fmt(SWITCH_vexp(ir));
        m_ir2fmt.set(IR_id(ir), FAABBBBBBBBv);
        return;
    case IR_ARRAY:
        ASSERT0(((CArray*)ir)->getDimNum() == 1);
        comp_ir_fmt(ARR_base(ir));
        comp_ir_fmt(ARR_sub_list(ir));
        return;
    case IR_CVT:
        comp_ir_fmt(CVT_exp(ir));
        return;
    case IR_PR:
        return;
    case IR_TRUEBR:
    case IR_FALSEBR:
        {
            IR * det = BR_det(ir);
            comp_ir_fmt(BR_det(ir));
            ASSERT0(det->is_relation());
            if (BIN_opnd1(det)->is_const()) {
                //AABBBB
                ASSERT0(BIN_opnd0(det)->is_pr());
                m_ir2fmt.set(IR_id(ir), FAABBBBv);
            } else {
                ASSERT0(BIN_opnd0(det)->is_pr());
                ASSERT0(BIN_opnd1(det)->is_pr());
                //ABCCCC
                m_ir2fmt.set(IR_id(ir), FABCCCCv);
            }
        }
        return;
    case IR_RETURN:
        ASSERT0(cnt_list(RET_exp(ir)) <= 1);
        if (RET_exp(ir) == NULL) {
            m_ir2fmt.set(IR_id(ir), F0);
        } else {
            ASSERT0(RET_exp(ir)->is_pr());
            m_ir2fmt.set(IR_id(ir), FAA);
        }
        return;
    case IR_SELECT:
        comp_ir_fmt(SELECT_pred(ir));
        comp_ir_fmt(SELECT_trueexp(ir));
        comp_ir_fmt(SELECT_falseexp(ir));
        UNREACH();
        return;
    case IR_REGION:
    default: UNREACH();
    }
}


void RSC::comp_ir_constrain()
{
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        for (IR const* ir = BB_first_ir(bb);
             ir != NULL; ir = BB_next_ir(bb)) {
            comp_ir_fmt(ir);
        }
    }
}


void RSC::dump_ir_fmt()
{
    if (g_tfile == NULL) { return; }
    fprintf(g_tfile, "\n==------- DUMP IR FMT --------==");
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        fprintf(g_tfile, "\n-- BB%d --", BB_id(bb));
        for (IR const* ir = BB_first_ir(bb);
             ir != NULL; ir = BB_next_ir(bb)) {
            FMT f = m_ir2fmt.get(IR_id(ir));
            fprintf(g_tfile, "\nFMT:%s", g_fmt_name[f]);
            dump_ir(ir, m_ru->get_type_mgr());
        }
    }
    fprintf(g_tfile, "\n");
    fflush(g_tfile);
}


void RSC::dump_glt_usable()
{
    if (g_tfile == NULL) { return; }
    fprintf(g_tfile, "\n=== DUMP GLT Usable Regs: ===");
    Vector<GLT*> * gltv = m_gltm->get_gltvec();
    bool dump_bit = false;
    for (INT i = 0; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        fprintf(g_tfile, "\nGLT%d(pr%d):", GLT_id(g), GLT_prno(g));
        if (GLT_prefer_reg(g) != REG_UNDEF) {
            fprintf(g_tfile, "prefer_reg=%d", LT_prefer_reg(g));
        } else {
            BitSet const* usable = GLT_usable(g);
            if (usable == NULL || usable->is_empty()) {
                fprintf(g_tfile, "--");
                continue;
            }

            if (dump_bit) {
                for (INT i = usable->get_first();
                     i >= 0; i = usable->get_next(i)) {
                    fprintf(g_tfile, "%d,", i);
                }
            } else {
                fprintf(g_tfile, "%d~%d",
                        usable->get_first(), usable->get_last());
            }
        }
    }
    fprintf(g_tfile, "\n");
    fflush(g_tfile);
}


void RSC::dump_bb(UINT bbid)
{
    if (g_tfile == NULL) { return; }
    fprintf(g_tfile, "\n-- BB%d Usable Regs --", bbid);
    LTMgr * ltm = m_gltm->get_ltm(bbid);
    ASSERT0(ltm);
    Vector<LT*> * ltvec = ltm->get_lt_vec();
    bool dump_bit = false;
    for (INT i = 0; i <= ltvec->get_last_idx(); i++) {
        LT const* lt = ltvec->get(i);
        if (lt == NULL) { continue; }
        fprintf(g_tfile, "\nLT%d(pr%d):", LT_uid(lt), LT_prno(lt));
        if (LT_prefer_reg(lt) != REG_UNDEF) {
            fprintf(g_tfile, "prefer_reg=%d", LT_prefer_reg(lt));
        } else {
            BitSet const* usable = LT_usable(lt);
            if (usable == NULL || usable->is_empty()) {
                fprintf(g_tfile, "--");
                continue;
            }
            if (dump_bit) {
                for (INT i = usable->get_first();
                     i >= 0; i = usable->get_next(i)) {
                    fprintf(g_tfile, "%d,", i);
                }
            } else {
                fprintf(g_tfile, "%d~%d", usable->get_first(), usable->get_last());
            }
        }
    }
    fflush(g_tfile);
}


void RSC::dump()
{
    if (g_tfile == NULL) { return; }
    dump_ir_fmt();
    dump_glt_usable();
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        dump_bb(BB_id(bb));
    }
    fflush(g_tfile);
}


void RSC::init_usable()
{
    m_usable[FAB][1] = get_4();
    m_usable[FAB][0] = get_4();

    m_usable[FABcv][1] = get_4();
    m_usable[FABcv][0] = NULL;

    m_usable[FAA][1] = get_8();
    m_usable[FAA][0] = get_8();

    m_usable[FAAv][1] = NULL;
    m_usable[FAAv][0] = NULL;

    m_usable[FAAAAv][1] = NULL;
    m_usable[FAAAAv][0] = NULL;

    m_usable[FAABBBB][1] = get_8();
    m_usable[FAABBBB][0] = get_16();

    m_usable[FAABBBBv][1] =
    m_usable[FAABBBBcv][1] =
    m_usable[FAABBBBcvh][1] =
    m_usable[FAABBCC][1] =
    m_usable[FAABBCCcv][1] = get_8();

    m_usable[FAABBBBv][0] =
    m_usable[FAABBBBcv][0] =
    m_usable[FAABBBBcvh][0] =
    m_usable[FAABBCC][0] =
    m_usable[FAABBCCcv][0] = get_8();

    m_usable[FABCCCCv][1] =
    m_usable[FABCCCCcv][1] = get_4();
    m_usable[FABCCCCv][0] =
    m_usable[FABCCCCcv][0] = get_4();

    m_usable[FAAAABBBB][1] = get_16();
    m_usable[FAAAABBBB][0] = get_16();

    m_usable[FAAAAAAAAv][1] = NULL;
    m_usable[FAAAAAAAAv][0] = NULL;

    m_usable[FAABBBBBBBBv][1] = get_8();
    m_usable[FAABBBBBBBBv][0] = get_8();

    m_usable[FAABBBBBBBBcv][1] = get_8();
    m_usable[FAABBBBBBBBcv][0] = get_8();

    m_usable[FACDEFGBBBBv][1] = get_4();
    m_usable[FACDEFGBBBBv][0] = get_4();

    m_usable[FAACCCCBBBBv][1] = get_16();
    m_usable[FAACCCCBBBBv][0] = get_16();

    m_usable[FAABBBBBBBBBBBBBBBBcv][1] = get_16();
    m_usable[FAABBBBBBBBBBBBBBBBcv][0] = get_16();

    ASSERT0(FAABBBBBBBBBBBBBBBBcv + 1 == FNUM);
}


//Compute usable for lt.
void RSC::comp_lt_usable(LT * lt, LTMgr * ltm)
{
    BitSet * occ = LT_occ(lt);
    if (occ == NULL) { return; }

    BitSet const* usable = NULL;
    bool first_alloc = true;
    for (INT i = occ->get_first(); i >= 0; i = occ->get_next(i)) {
        IR const* ir = ltm->get_ir(i);
        ASSERT0(ir);
        BitSet * x = get_usable(get_fmt(ir), lt->is_def(i));
        if (x == NULL) { continue; }
        if (usable == NULL) {
            if (first_alloc) {
                first_alloc = false;
                usable = x;
            } else {
                ;//usable is empty!
            }
        } else if (usable == x || usable->is_contain(*x, true)) {
            //Shrink
            usable = x;
        }
    }

    if (LT_is_global(lt)) {
        GLT * glt = mapLT2GLT(lt);
        ASSERT0(glt && GLT_usable(glt));
        if (GLT_usable(glt)->is_contain(*usable, true)) {
            //Shrink
            GLT_usable(glt) = usable;
        }

        //Note all local part of global lt will be set after local computation
        //complete. The assignment to LT_usable() may be redundant.
        LT_usable(lt) = GLT_usable(glt);
    } else {
        LT_usable(lt) = usable;
    }
}


//Verify each stmt has instruction constrain.
bool RSC::verify_fmt()
{
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        for (IR const* ir = BB_first_ir(bb);
             ir != NULL; ir = BB_next_ir(bb)) {
            FMT f = m_ir2fmt.get(IR_id(ir));
            UNUSED(f);
            ASSERT0(f != FUNDEF);
        }
    }
    return true;
}


//'omit_constrain': true if all constrain is omit, and
//                    only initialize usable set to the largest.
void RSC::comp_local_usage(LTMgr * ltm, bool only_local, bool omit_constrain)
{
    Vector<LT*> * ltvec = ltm->get_lt_vec();
    for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
        LT * lt = ltvec->get(i);
        if (lt == NULL) { continue; }
        if (only_local && LT_is_global(lt)) { continue; }
        if (omit_constrain) {
            LT_usable(lt) = get_16();
            continue;
        }
        comp_lt_usable(lt, ltm);
    }
}


//Recompute all global and local lt constrain.
//Note an explictly clean() is dispensable before this call.
void RSC::perform(bool omit_constrain)
{
    comp_ir_constrain();
    ASSERT0(verify_fmt());

    //Initialize global lt usable-regs.
    Vector<GLT*> * gltv = m_gltm->get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        GLT_usable(g) = get_16();
    }

    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm->map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        comp_local_usage(ltm, false, omit_constrain);
    }
    if (omit_constrain) { return; }

    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        g->set_local(*m_gltm);
    }
}
//END RSC


//
//START GltMgr Global Life Time Manager
//
GltMgr::GltMgr(Region * ru, PRDF * prdf, RA * ra)
{
    m_glt_count = 1;
    m_ru = ru;
    m_ra = ra;
    m_rsc = ra->get_rsc();
    m_tm = ru->get_type_mgr();
    m_prdf = prdf;
    m_is_consider_local_interf = false;
    m_pool = smpoolCreate(sizeof(GLT) * 10, MEM_COMM);
}


//Localize glt to several lts.
void GltMgr::localize(GLT * g)
{
    ASSERT0(!GLT_is_param(g));
    DefDBitSetCore * bbs = GLT_bbs(g);
    ASSERT(bbs, ("should not localize an empty glt"));
    SEGIter * sc = NULL;
    UINT prno = GLT_prno(g);
    for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
        LTMgr * ltm = get_ltm(j);
        if (ltm == NULL) { continue; }
        DefSBitSetCore * livein = m_prdf->get_livein(j);
        DefSBitSetCore * liveout = m_prdf->get_liveout(j);
        if (livein != NULL) {
            livein->diff(prno, m_prdf->getMiscBitSetMgr());
        }
        if (liveout != NULL) {
            liveout->diff(prno, m_prdf->getMiscBitSetMgr());
        }
        LT * gl = ltm->map_pr2lt(prno);
        ASSERT0(gl); //glt miss local part.
        //Note the usable-regs of local part do not changed.

        bool has_occ = true;
        if (LT_occ(gl) == NULL || LT_occ(gl)->is_empty()) {
            ltm->removeLT(gl);
            has_occ = false;
        } else {
            LT_is_global(gl) = false;
        }

        ltm->clean();
        ltm->build(true, NULL, m_cii);

        ltm->get_ig()->erase();
        ltm->get_ig()->build();
        //m_rsc->comp_local_usage(ltm, true); Does it need?

        if (has_occ) {
            //Set perfer reg.
            gl = ltm->map_pr2lt(prno); //recheck
            ASSERT0(!LT_is_global(gl));
            LT_prefer_reg(gl) = FIRST_PHY_REG;
        }
    }
    m_pr2glt.set(prno, NULL);
    m_gltid2glt_map.set(GLT_id(g), NULL);
    m_sbs_mgr.free_dbitsetc(bbs);
}


//Build glt that lifetime just likes 'cand'.
//Note the function only build global part of lt.
GLT * GltMgr::buildGltLike(IR * pr, GLT * cand)
{
    GLT * newglt = new_glt(PR_no(pr));
    DefDBitSetCore * bbs = GLT_bbs(cand);
    if (bbs != NULL) {
        GLT_bbs(newglt) = m_sbs_mgr.create_dbitsetc();
        GLT_bbs(newglt)->copy(*bbs, m_sbs_mgr);
        SEGIter * sc = NULL;
        UINT candprno = GLT_prno(cand);
        UINT prno = GLT_prno(newglt);
        for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
            DefSBitSetCore * livein = m_prdf->get_livein(j);
            DefSBitSetCore * liveout = m_prdf->get_liveout(j);
            if (livein != NULL && livein->is_contain(candprno)) {
                livein->bunion(prno, m_prdf->getMiscBitSetMgr());
            }
            if (liveout != NULL && liveout->is_contain(candprno)) {
                liveout->bunion(prno, m_prdf->getMiscBitSetMgr());
            }
        }
    }
    return newglt;
}


//Verify the consistency of global lt and local part.
bool GltMgr::verify()
{
    for (INT i = 1; i <= m_gltid2glt_map.get_last_idx(); i++) {
        GLT * g = m_gltid2glt_map.get(i);
        if (g == NULL || GLT_bbs(g) == NULL) { continue; }
        SEGIter * cur = NULL;
        for (INT j = GLT_bbs(g)->get_first(&cur);
             j >= 0; j = GLT_bbs(g)->get_next(j, &cur)) {
            LTMgr * ltm = m_bb2ltmgr.get(j);
            ASSERT0(ltm);
            LT * l = ltm->map_pr2lt(GLT_prno(g));
            UNUSED(l);
            ASSERT0(l && LT_is_global(l));
            ASSERT0(LT_phy(l) == GLT_phy(g));

            //Global lt's reg group size may be larger than local part.
            ASSERT0(LT_rg_sz(l) <= GLT_rg_sz(g));
        }
    }
    return true;
}


//Dump global lt info.
void GltMgr::dumpg()
{
    if (g_tfile == NULL) return;
    BitSet prs;
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        for (IR const* o = BB_first_ir(bb);
             o != NULL; o = BB_next_ir(bb)) {
            m_cii.clean();
            for (IR const* k = iterInitC(o, m_cii);
                 k != NULL; k = iterNextC(m_cii)) {
                if (k->is_pr()) {
                    prs.bunion(PR_no(k));
                }
            }
        }
    }
    INT maxprno = MAX(0, prs.get_last());

    UINT maxr = 0;
    for (INT i = prs.get_first(); i >= 0; i = prs.get_next(i)) {
        GLT * g = map_pr2glt(i);
        if (g != NULL && g->has_allocated()) {
            maxr = MAX(maxr, GLT_phy(g));
        }
    }

    CHAR litbuf[32];
    UINT num = 0; //number of allocated register.
    while (maxr != 0) {
        maxr /= 10;
        num++;
    }
    sprintf(litbuf, "v%%-%dd", num);

    CHAR litbuf2[32];
    UINT num2 = 0; //number of pr.
    while (maxprno != 0) {
        maxprno /= 10;
        num2++;
    }
    sprintf(litbuf2, "pr%%-%dd", num2);

    fprintf(g_tfile,
            "\n=== DUMP Global Life Time = maxreg:%d = paramnum:%d ===",
            m_ra->m_maxreg, m_ra->m_param_num);
    if (m_ra->m_param_num > 0) {
        fprintf(g_tfile, "{");
        for (INT i = 0; i <= m_params.get_last_idx(); i++) {
            INT pr = m_params.get(i);
            if (pr != 0) {
                fprintf(g_tfile, "pr%d", pr);
                GLT * g = map_pr2glt(pr);
                ASSERT0(g); //may be pr is local.
                if (g->has_allocated()) {
                    fprintf(g_tfile, "(v%d)", GLT_phy(g));
                }
                fprintf(g_tfile, ",");
            } else {
                fprintf(g_tfile, "--,");
            }
        }
        fprintf(g_tfile, "}");
    }
    fprintf(g_tfile, "\n");

    CHAR buf[64];
    for (INT i = prs.get_first(); i >= 0; i = prs.get_next(i)) {
        GLT * g = map_pr2glt(i);
        if (g == NULL) { continue; }

        //Print prno.
        fprintf(g_tfile, "\n");
        fprintf(g_tfile, litbuf2, GLT_prno(g));
        fprintf(g_tfile, " ");

        //Print phy.
        if (g != NULL) {
            fprintf(g_tfile, "[");
            if (GLT_phy(g) == REG_UNDEF) {
                UINT h = 0;
                while (h <= num) {
                    fprintf(g_tfile, "-");
                    h++;
                }
            } else {
                fprintf(g_tfile, litbuf, GLT_phy(g));
            }
            fprintf(g_tfile, "]");
        }

        //Print live BB.
        DefDBitSetCore * livebbs = GLT_bbs(g);
        if (livebbs == NULL || livebbs->is_empty()) { continue; }
        INT start = 0;
        SEGIter * sc = NULL;
        for (INT u = livebbs->get_first(&sc);
             u >= 0; u = livebbs->get_next(u, &sc)) {
            for (INT j = start; j < u; j++) {
                sprintf(buf, "%d,", j);
                for (UINT k = 0; k < strlen(buf); k++) {
                    fprintf(g_tfile, " ");
                }
            }
            fprintf(g_tfile, "%d,", u);
            start = u + 1;
        }
    }
    fprintf(g_tfile, "\n");
    fflush(g_tfile);
}


//Dump global and local lt info.
void GltMgr::dump()
{
    dumpg();
    if (g_tfile == NULL) { return; }
    fprintf(g_tfile, "\n=== DUMP Local Life Time ===");
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * lltmgr = map_bb2ltm(bb);
        lltmgr->dump();
    }
    fprintf(g_tfile, "\n");
    fflush(g_tfile);
}


//Dump local lt info.
void GltMgr::dumpl(UINT bbid)
{
    LTMgr * l = m_bb2ltmgr.get(bbid);
    ASSERT0(l);
    l->dump();
}


LTMgr * GltMgr::map_bb2ltm(IRBB * bb)
{
    LTMgr * l = m_bb2ltmgr.get(BB_id(bb));
    if (l == NULL) {
        //Allocated object managed by RaMgr, and do not delete it youself.
        l = new LTMgr(bb, m_prdf, this, m_pool);
        m_bb2ltmgr.set(BB_id(bb), l);
    }
    return l;
}


//Every OR which refering sr must be assigned to same cluster, therefore
//the only time to record cluster information is the first meeting with sr.
GLT * GltMgr::new_glt(UINT prno)
{
    GLT * glt = (GLT*)xmalloc(sizeof(GLT));
    GLT_id(glt) = m_glt_count++;
    GLT_prno(glt) = prno;
    GLT_phy(glt) = REG_UNDEF;
    GLT_prefer_reg(glt) = REG_UNDEF;
    GLT_rg_sz(glt) = 1;
    GLT_freq(glt) = 1.0;
    m_pr2glt.set(prno, glt);
    m_gltid2glt_map.set(GLT_id(glt), glt);
    return glt;
}


void GltMgr::renameGLT(GLT * g)
{
    IR * newpr = NULL;
    SEGIter * cur = NULL;
    if (GLT_bbs(g) == NULL) { return; }

    for (INT i = GLT_bbs(g)->get_first(&cur);
         i >= 0; i = GLT_bbs(g)->get_next(i, &cur)) {
        LTMgr * ltm = m_bb2ltmgr.get(i);
        ASSERT0(ltm);
        LT * l = ltm->map_pr2lt(GLT_prno(g));
        ASSERT0(l);
        ltm->renameLT(l, &newpr);
    }
    if (newpr != NULL) {
        GLT_prno(g) = PR_no(newpr);
        m_pr2glt.set(GLT_prno(g), g);
    }
    m_ru->freeIR(newpr);
}


//If local PR is same as global PR, rename local PR.
//If there are multiple local lifetime corresponded to same PR, rename them.
void GltMgr::renameLocal()
{
    TMap<UINT, LT*> prno2lt;
    BitSet met;
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_bb2ltmgr.get(BB_id(bb));
        if (ltm == NULL) { continue; }
        ltm->rename(prno2lt, met);
    }
}


void GltMgr::rename()
{
    renameLocal();
    TMap<UINT, GLT*> prno2glt;
    for (INT i = 1; i <= m_gltid2glt_map.get_last_idx(); i++) {
        GLT * g = m_gltid2glt_map.get(i);
        ASSERT0(g);
        GLT * prior = prno2glt.get(GLT_prno(g));
        if (prior == NULL) {
            prno2glt.set(GLT_prno(g), g);
        } else if (prior != g) {
            renameGLT(g);
        }
    }
}


//Build global life time and set map between PR and Vreg parameter.
void GltMgr::build(bool build_group_part)
{
    List<IRBB*> * bbl = m_ru->get_bb_list();
    INT vreg =     m_ra->m_vregnum - m_ra->m_param_num;
    for (UINT i = 0; i < m_ra->m_param_num; i++, vreg++) {
        ASSERT0(vreg >= 0 && vreg < (INT)m_ra->m_vregnum);
        IR * pr = m_ra->m_v2pr->get(vreg);
        if (pr == NULL) {
            //Some parameter has no use.
            continue;
        }
        UINT prno = PR_no(pr);
        GLT * glt = map_pr2glt(prno);
        if (glt == NULL) {
            glt = new_glt(prno);
        }
        GLT_is_param(glt) = true;
        GLT_param_pos(glt) = (BYTE)i;
        m_params.set(i, prno);
    }
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        DefSBitSetCore * livein = m_prdf->get_livein(BB_id(bb));
        DefSBitSetCore * liveout = m_prdf->get_liveout(BB_id(bb));
        if (livein != NULL) {
            SEGIter * cur = NULL;
            for (INT i = livein->get_first(&cur);
                 i >= 0; i = livein->get_next(i, &cur)) {
                GLT * glt = map_pr2glt(i);
                if (glt == NULL) {
                    glt = new_glt(i);
                }
                if (GLT_bbs(glt) == NULL) {
                    GLT_bbs(glt) = m_sbs_mgr.create_dbitsetc();
                }
                GLT_bbs(glt)->bunion(BB_id(bb), m_sbs_mgr);
            }
        }

        if (liveout != NULL) {
            SEGIter * cur = NULL;
            for (INT i = liveout->get_first(&cur);
                 i >= 0; i = liveout->get_next(i, &cur)) {
                GLT * glt = map_pr2glt(i);
                if (glt == NULL) {
                    glt = new_glt(i);
                }
                if (GLT_bbs(glt) == NULL) {
                    GLT_bbs(glt) = m_sbs_mgr.create_dbitsetc();
                }
                GLT_bbs(glt)->bunion(BB_id(bb), m_sbs_mgr);
            }
        }

        LTMgr * lltmgr = map_bb2ltm(bb);
        lltmgr->build(m_is_consider_local_interf, NULL, m_cii);
    }
    rename();

    if (!build_group_part) {
        return;
    }

    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * lltmgr = map_bb2ltm(bb);
        lltmgr->buildGroup(m_cii);
    }
    renameLocal(); //renaming group part lts.
}
//END GltMgr


//
//START GIG
//
void GIG::dump_vcg(CHAR const* name)
{
    if (name == NULL) {
        name = "graph_global_if.vcg";
    }
    unlink(name);
    FILE * h = fopen(name, "a+");
    ASSERT(h, ("%s create failed!!!",name));
    fprintf(h, "graph: {"
              "title: \"Graph\"\n"
              "shrink:  15\n"
              "stretch: 27\n"
              "layout_downfactor: 1\n"
              "layout_upfactor: 1\n"
              "layout_nearfactor: 1\n"
              "layout_splinefactor: 70\n"
              "spreadlevel: 1\n"
              "treefactor: 0.500000\n"
              "node_alignment: center\n"
              "orientation: top_to_bottom\n"
              "late_edge_labels: no\n"
              "display_edge_labels: yes\n"
              "dirty_edge_labels: no\n"
              "finetuning: no\n"
              "nearedges: no\n"
              "splines: yes\n"
              "ignoresingles: no\n"
              "straight_phase: no\n"
              "priority_phase: no\n"
              "manhatten_edges: no\n"
              "smanhatten_edges: no\n"
              "port_sharing: no\n"
              "crossingphase2: yes\n"
              "crossingoptimization: yes\n"
              "crossingweight: bary\n"
              "arrow_mode: free\n"
              "layoutalgorithm: mindepthslow\n"
              "node.borderwidth: 2\n"
              "node.color: lightcyan\n"
              "node.textcolor: black\n"
              "node.bordercolor: blue\n"
              "edge.color: darkgreen\n");

    CHAR buf[MAX_OR_SR_NAME_BUF_LEN];
    //Print node
    INT c;
    for (Vertex const* v = m_vertices.get_first(c);
         v != NULL;  v = m_vertices.get_next(c)) {
        GLT * glt = m_gltm->get_glt(VERTEX_id(v));
        sprintf(buf, "GLT%d(pr%d):", VERTEX_id(v), GLT_prno(glt));
        fprintf(h, "\nnode: { title:\"%d\" label:\"%s\" "
                   "shape:circle fontname:\"courB\" color:gold}",
                VERTEX_id(v), buf);
    }

    //Print edge
    for (Edge const* e = m_edges.get_first(c);
         e != NULL;  e = m_edges.get_next(c)) {
        fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
                VERTEX_id(EDGE_from(e)),
                VERTEX_id(EDGE_to(e)),
                m_is_direction ? "" : "arrowstyle:none" );
    }
    fprintf(h, "\n}\n");
    fclose(h);
}


//Return true if phy has been occupied by g's neighbours.
//'nis': for tmp use.
bool GIG::is_interf_with_neighbour(GLT * g, DefSBitSet & nis, UINT phy)
{
    ASSERT0(g && phy < 65000);
    nis.clean();
    get_neighbor_set(nis, GLT_id(g));
    SEGIter * cur = NULL;
    for (INT ltid = nis.get_first(&cur);
         ltid >= 0; ltid = nis.get_next(ltid, &cur)) {
        GLT * g = m_gltm->get_glt(ltid);
        ASSERT0(g);
        if (!g->has_allocated()) { continue; }
        if (GLT_phy(g) == phy) { return true; }
        if (GLT_rg_sz(g) > 1) {
            ASSERT0(GLT_rg_sz(g) == RG_PAIR_SZ);
            if ((UINT)GLT_phy(g) + 1 == phy) { return true; }
        }
    }
    return false;
}


bool GIG::is_interf(IN GLT * glt1, IN GLT * glt2)
{
    if (GLT_id(glt1) == GLT_id(glt2)) return true;
    if (GLT_bbs(glt1) == NULL || GLT_bbs(glt2) == NULL) {
        return false;
    }
    if (!GLT_bbs(glt1)->is_intersect(*GLT_bbs(glt2))) {
        return false;
    }
    if (!m_is_consider_local_interf) { return true; }
    DefDBitSetCore * bs1 = GLT_bbs(glt1);
    DefDBitSetCore * bs2 = GLT_bbs(glt2);
    UINT pr1 = GLT_prno(glt1);
    UINT pr2 = GLT_prno(glt2);
    SEGIter * sc = NULL;
    for (INT i = bs1->get_first(&sc); i >= 0; i = bs1->get_next(i, &sc)) {
        if (!bs2->is_contain(i)) { continue; }
        IRBB * bb = m_cfg->get_bb(i);
        ASSERT0(bb != NULL);
        LTMgr * ltmgr = m_gltm->map_bb2ltm(bb);
        LT * lt1 = ltmgr->map_pr2lt(pr1);
        LT * lt2 = ltmgr->map_pr2lt(pr2);
        ASSERT0(lt1 != NULL);
        ASSERT0(lt2 != NULL);
        if (lt1->is_intersect(lt2)) {
            return true;
        }
    }
    return false;
}


//Set glt interfered with a list of glt.
void GIG::set_interf_with(UINT gltid, List<UINT> & lst)
{
    ASSERT(m_vertices.find((OBJTY)gltid), ("not on graph"));
    UINT n = lst.get_elem_count();
    for (UINT i = lst.get_head(); n > 0; i = lst.get_next(), n--) {
        ASSERT(m_vertices.find((OBJTY)i), ("not on graph"));
        addEdge(gltid, i);
    }
}


void GIG::build()
{
    //Check interference
    Vector<GLT*> * pr2glt = m_gltm->get_pr2glt_map();
    INT n = pr2glt->get_last_idx();
    for (INT i = 0; i <= n; i++) {
        GLT * lt1 = pr2glt->get(i);
        if (lt1 == NULL) { continue; }
        addVertex(GLT_id(lt1));
        for (INT j = i + 1; j <= n; j++) {
            GLT * lt2 = pr2glt->get(j);
            if (lt2 == NULL) { continue; }
            if (is_interf(lt1, lt2)) {
                addEdge(GLT_id(lt1), GLT_id(lt2));
            }
        }
    }
}
//END GIG


//
//START IG
//
bool IG::is_interf(LT const* lt1, LT const* lt2) const
{
    if (lt1 == lt2) { return true; }
    if (LT_range(lt1) == NULL || LT_range(lt2) == NULL) {
        return false;
    }
    if (LT_range(lt1)->is_intersect(*LT_range(lt2))) {
        return true;
    }
    return false;
}


void IG::build()
{
    ASSERT0(m_ltm);
    Vector<LT*> * vec = m_ltm->get_lt_vec();
    INT n = vec->get_last_idx();
    for (INT i = 1; i <= n; i++) {
        LT const* lt1 = vec->get(i);
        if (lt1 == NULL) { continue; }
        addVertex(LT_uid(lt1));
        for (INT j = i + 1; j <= n; j++) {
            LT const* lt2 = vec->get(j);
            if (lt2 == NULL) { continue; }
            if (is_interf(lt1, lt2)) {
                addEdge(LT_uid(lt1), LT_uid(lt2));
            }
        }
    }
}


void IG::get_neighbor(OUT List<LT*> & nis, LT * lt) const
{
    nis.clean();
    //Ensure VertexHash::find is readonly.
    IG * pthis = const_cast<IG*>(this);
    Vertex * vex  = pthis->m_vertices.find((OBJTY)LT_uid(lt));
    if (vex == NULL) return;

    EdgeC * el = VERTEX_in_list(vex);
    while (el != NULL) {
        INT v = VERTEX_id(EDGE_from(EC_edge(el)));
        LT * ni = m_ltm->get_lt(v);
        ASSERT0(ni);
        if (!nis.find(ni)) {
            nis.append_tail(ni);
        }
        el = EC_next(el);
    }

    el = VERTEX_out_list(vex);
    while (el != NULL) {
        INT v = VERTEX_id(EDGE_to(EC_edge(el)));
        LT * ni = m_ltm->get_lt(v);
        ASSERT0(ni);
        if (!nis.find(ni)) {
            nis.append_tail(ni);
        }
        el = EC_next(el);
    }
}


void IG::dump_vcg(CHAR const* name)
{
    if (name == NULL) {
        name = "graph_local_if.vcg";
    }
    unlink(name);
    FILE * h = fopen(name, "a+");
    ASSERT(h, ("%s create failed!!!",name));
    fprintf(h, "graph: {"
              "title: \"Graph\"\n"
              "shrink:  15\n"
              "stretch: 27\n"
              "layout_downfactor: 1\n"
              "layout_upfactor: 1\n"
              "layout_nearfactor: 1\n"
              "layout_splinefactor: 70\n"
              "spreadlevel: 1\n"
              "treefactor: 0.500000\n"
              "node_alignment: center\n"
              "orientation: top_to_bottom\n"
              "late_edge_labels: no\n"
              "display_edge_labels: yes\n"
              "dirty_edge_labels: no\n"
              "finetuning: no\n"
              "nearedges: no\n"
              "splines: yes\n"
              "ignoresingles: no\n"
              "straight_phase: no\n"
              "priority_phase: no\n"
              "manhatten_edges: no\n"
              "smanhatten_edges: no\n"
              "port_sharing: no\n"
              "crossingphase2: yes\n"
              "crossingoptimization: yes\n"
              "crossingweight: bary\n"
              "arrow_mode: free\n"
              "layoutalgorithm: mindepthslow\n"
              "node.borderwidth: 2\n"
              "node.color: lightcyan\n"
              "node.textcolor: black\n"
              "node.bordercolor: blue\n"
              "edge.color: darkgreen\n");

    CHAR buf[MAX_OR_SR_NAME_BUF_LEN];
    ASSERT0(m_ltm);
    //Print node
    INT c;
    for (Vertex const* v = m_vertices.get_first(c);
         v != NULL;  v = m_vertices.get_next(c)) {
        LT * lt = m_ltm->get_lt(VERTEX_id(v));
        sprintf(buf, "LT%d(pr%d):", VERTEX_id(v), LT_prno(lt));
        fprintf(h, "\nnode: { title:\"%d\" label:\"%s\" shape:circle fontname:\"courB\" color:gold}",
                VERTEX_id(v), buf);
    }

    //Print edge
    for (Edge const* e = m_edges.get_first(c);
         e != NULL;  e = m_edges.get_next(c)) {
        fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
                VERTEX_id(EDGE_from(e)),
                VERTEX_id(EDGE_to(e)),
                m_is_direction ? "" : "arrowstyle:none" );
    }
    fprintf(h, "\n}\n");
    fclose(h);
}
//END IG


//
//START LTMgr
//
LTMgr::LTMgr(IRBB * bb, PRDF * prdf, GltMgr * gltm, SMemPool * pool)
{
    m_bb = bb;
    m_pool = pool;
    m_prdf = prdf;
    m_gltm = gltm;
    m_tm = gltm->m_tm;
    m_ru = gltm->m_ru;
    m_ra = gltm->m_ra;
    m_lt_count = 1;
    m_pr2v = m_ra->m_pr2v;
    m_v2pr = m_ra->m_v2pr;
    m_ig.set_ltm(this);
}


/* Generate spill instruction at 'pos',
pos may be the liveout/livein pos.

Spilling for Def.
    e.g:
        sr1 = ...

    after spilling:

    sr1 = ...
    [spill_loc] = sr1

NOTICE:
    1. Each of spilling code generated are executed unconditionally.
       Allow to spill unallocated 'lt'.
    2. position in LT should be updated. */
IR * LTMgr::genSpill(LT * lt, INT pos)
{
    IR * spill_loc = m_ru->buildPR(m_tm->getSimplexType(D_I32));
    IR * ltpr = m_ru->buildPR(m_tm->getSimplexType(D_I32));
    PR_no(ltpr) = LT_prno(lt);
    IR * spill = m_ru->buildStorePR(PR_no(spill_loc),
                                      IR_dt(spill_loc), ltpr);
    if (pos == (INT)get_first_pos()) {
        //Prepend store at start pos of BB.
        BB_irlist(m_bb).append_head(spill);
    } else if (pos == (INT)get_last_pos()) {
        //Append store before the last non-boundary stmt of BB.
        //e.g: reload should insert be call/branch.
        IR * lastir = BB_last_ir(m_bb);
        if (lastir != NULL && lastir->is_calls_stmt()) {
            BB_irlist(m_bb).append_tail(spill);
        } else {
            ASSERT(!lastir->is_cond_br() && !lastir->is_uncond_br(),
                    ("How to spill after branch instruction."));
            BB_irlist(m_bb).append_tail_ex(spill);
        }
    } else {
        IR * marker = get_ir(pos);
        ASSERT0(marker);
        BB_irlist(m_bb).insert_after(spill, marker);
    }
    m_ra->get_rsc()->comp_ir_fmt(spill);
    return spill_loc;
}


/* Generate spill instruction after 'marker' and swap pr.
Return spill location.
'stpr': indicate the result register to be spilled.
    It is also stmt marker to indicate where the spill instruction insert.
'prno': prno that to be spilled.
'spill_loc': spill location.

e.g: pr1 = pr2 + 3
    =>
     [spill_loc] = pr2 + 3
     pr1 = [spill_loc] */
IR * LTMgr::genSpillSwap(IR * stmt, UINT prno, Type const* prty, IR * spill_loc)
{
    ASSERT0(stmt && (stmt->is_stpr() || stmt->is_calls_stmt()) && prty);

    //Generate and insert spilling operation.
    if (spill_loc == NULL) {
        spill_loc = m_ru->buildPR(prty);
    }
    IR * spill = m_ru->buildStorePR(prno, prty, spill_loc);
    m_ra->get_rsc()->comp_ir_fmt(spill);
    BB_irlist(m_bb).insert_after(spill, stmt);

    //Replace orginal PR to spill_loc.
    if (stmt->is_stpr()) {
        STPR_no(stmt) = PR_no(spill_loc);
    } else if (CALL_prno(stmt) == prno) {
        CALL_prno(stmt) = PR_no(spill_loc);
    }
    return spill_loc;
}


/* Generate spill instruction after 'marker'.
Return spill location.
'orgpr': register to be spilled.
'spill_loc': spill location.

e.g: pr1 = pr2 + 3
    =>
     pr1 = pr2 + 3
     [spill_loc] = pr1 */
IR * LTMgr::genSpill(UINT prno, Type const* type, IR * marker, IR * spill_loc)
{
    ASSERT0(prno > 0 && type && marker && marker->is_stmt());
    if (spill_loc == NULL) {
        spill_loc = m_ru->buildPR(type);
    }
    IR * spill = m_ru->buildStorePR(PR_no(spill_loc), IR_dt(spill_loc),
                                      m_ru->buildPRdedicated(prno, type));
    BB_irlist(m_bb).insert_after(spill, marker);
    m_ra->get_rsc()->comp_ir_fmt(spill);
    return spill_loc;
}


/* Generate reload instruction at 'pos',
pos may be the liveout/livein pos.

Reloading for Use.
    e.g:
        ... = sr1
    after reloading:
        sr2 = [spill_loc]
        ... = sr2
Return the new sr generated.

'ors': if it is NOT NULL, return the ORs generated.

NOTICE:
    Each reloads are executed unconditionally. */
IR * LTMgr::genReload(LT * lt, INT pos, IR * spill_loc)
{
    IR * ltpr = m_ru->buildPR(IR_dt(spill_loc));
    if (LT_is_global(lt)) {
        //Keep original PR unchanged.
        PR_no(ltpr) = LT_prno(lt);
    }
    IR * reload = m_ru->buildStorePR(PR_no(ltpr), IR_dt(ltpr), spill_loc);
    m_ra->get_rsc()->comp_ir_fmt(reload);
    if (pos == (INT)get_first_pos()) {
        //Prepend reload at start pos of BB.
        BB_irlist(m_bb).append_head(reload);
    } else if (pos == (INT)get_last_pos()) {
        //Append reload before the last non-boundary stmt of BB.
        IR * lastir = BB_last_ir(m_bb);
        if (lastir != NULL && lastir->is_calls_stmt()) {
            BB_irlist(m_bb).append_tail(reload);
        } else {
            ASSERT(!lastir->is_cond_br() && !lastir->is_uncond_br(),
                    ("How to spill after branch instruction."));
            if (!lastir->is_return()) {
                //If last ir is return, the reload is dispensable.
                BB_irlist(m_bb).append_tail_ex(reload);
            }
        }
    } else {
        IR * marker = get_ir(pos);
        ASSERT0(marker);
        BB_irlist(m_bb).insert_before(reload, marker);
    }
    return ltpr;
}


/* Generate reload instruction before 'marker', from 'spill_loc' to 'newpr'.
Reloading for Use.
    e.g:
        ... = sr1
    after reloading:
        sr1 = [spill_loc]
        ... = sr1
Return the result pr of reloading.
NOTICE:
    Each of reloading code generated are executed unconditionally. */
IR * LTMgr::genReload(IR * newpr, IR * marker, IR * spill_loc)
{
    ASSERT0(newpr && newpr->is_pr() &&
             marker && spill_loc && spill_loc->is_pr());
    IR * reload = m_ru->buildStorePR(PR_no(newpr), IR_dt(newpr),
                                       m_ru->dupIR(spill_loc));
    m_ra->m_rsc.comp_ir_fmt(reload);

    IRBB * irbb = marker->get_bb();
    ASSERT0(irbb);
    BB_irlist(irbb).insert_before(reload, marker);
    return newpr;
}


/* Generate reload instruction before 'marker', and
swap spill_loc and newpr.
Return the result pr of reloading. This pr need a phy register.
    e.g:
        ... = sr1  //the stmt is marker.
    after reloading:
        [spill_loc] = sr1
        ... = [spill_loc] //the stmt is marker.

NOTE: Caller is responsible for keeping spill_loc's id unique. */
IR * LTMgr::genReloadSwap(IR * orgpr, IR * marker)
{
    ASSERT0(marker && marker->is_stmt());
    ASSERT0(orgpr && orgpr->is_pr());
    UINT spill_prno = m_ru->buildPrno(IR_dt(orgpr));
    IR * reload = m_ru->buildStorePR(spill_prno, IR_dt(orgpr),
                                       m_ru->dupIR(orgpr));
    m_ra->m_rsc.comp_ir_fmt(reload);

    IRBB * irbb = marker->get_bb();
    ASSERT0(irbb);
    BB_irlist(irbb).insert_before(reload, marker);

    //To speed up compiling, we only change the prno as tricky.
    PR_no(orgpr) = spill_prno;
    return orgpr;
}


//Every OR which refering sr must be assigned to same cluster, therefore
//the only time to record cluster information is the first meeting with sr.
LT * LTMgr::newLT(UINT prno)
{
    ASSERT(m_max_lt_len > 0, ("Life time length is overrange."));
    ASSERT0(prno >= 1);
    LT * lt = (LT*)xmalloc(sizeof(LT));
    LT_uid(lt) = m_lt_count++;
    LT_range(lt) = m_gltm->get_bs_mgr()->create(0);
    m_prno2lt.set(prno, lt);
    m_lt_vec.set(LT_uid(lt), lt);

    LT_prno(lt) = prno;
    LT_phy(lt) = REG_UNDEF;
    LT_prefer_reg(lt) = REG_UNDEF;
    LT_ltg(lt) = NULL;
    LT_ltg_pos(lt) = 0;
    LT_rg_sz(lt) = 1;
    return lt;
}


//Life times which got same physical register with 'sr' must
//record the current occurrence.
void LTMgr::recordPhyRegOcc(LT * lt, UINT pos, IN BitSet & lived_lt)
{
    if (lt->has_allocated()) {
        //Record the occurrence before the lived life-time be
        //removed out of 'lived_lt'.
        for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
            LT * lived = get_lt(i);
            ASSERT0(lived);
            if (lt->has_allocated() &&
                LT_phy(lived) == LT_phy(lt)) {
                //Lifetime of lt died at current position.
                LT_range(lived)->bunion(pos);
            }
        }
    }
}


void LTMgr::processResultGroupPart(IR const* ir, UINT pos, OUT BitSet & lived_lt)
{
    ASSERT0(ir && ir->is_stpr());
    ASSERT0(is_pair(ir));
    LT * lt = map_pr2lt(STPR_no(ir));
    ASSERT0(lt);

    bool find = false;
    UINT vreg = m_pr2v->get(STPR_no(const_cast<IR*>(ir)), &find);
    IR * sib;
    if (find) {
        vreg++; //ir is always the lowest part of group.
        sib = m_v2pr->get(vreg);
        if (sib == NULL) {
            //low part is mapped during dex2ir, but high part may be not appear.
            //So its related IR is NULL.
            sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
        } else {
            sib = m_ru->dupIR(sib);
            IR_dt(sib) = m_tm->getSimplexTypeEx(D_U32);
        }
    } else if (LT_ltg(lt) != NULL) {
        //ir is generated by renaming. lt already has grouped.
        sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
        LT * pair = LT_ltg(lt)->get(1);
        ASSERT0(pair);
        PR_no(sib) = LT_prno(pair);
    } else {
        //ir is generated by renaming.
        sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
    }

    LT * siblt = processResultPR(PR_no(sib), pos, lived_lt);
    ASSERT0(siblt);
    genGroup(lt, siblt);
}


LT * LTMgr::processResultPR(UINT prno, UINT pos, OUT BitSet & lived_lt)
{
    LT * lt = map_pr2lt(prno);
    if (lt == NULL) {
        lt = newLT(prno);
    }
    if (LT_occ(lt) == NULL) {
        LT_occ(lt) = m_gltm->get_bs_mgr()->create(0);
    }
    LT_occ(lt)->bunion(pos);
    LT_range(lt)->bunion(pos);
    recordPhyRegOcc(lt, pos, lived_lt);
    lived_lt.diff(LT_uid(lt));

    //Phy register definition.
    if (lt->has_allocated()) {
        for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
            LT const* lived = get_lt(i);
            ASSERT0(lived);
            if (lived->has_allocated() &&
                lt->is_reg_equal(lived)) {
                lived_lt.diff(LT_uid(lived));
            }
        }//end for
    }
    return lt;
}


IR * LTMgr::genMappedPR(UINT vid, Type const* ty)
{
    IR * vx = m_ra->m_v2pr->get(vid);
    if (vx == NULL) {
        vx = m_ru->buildPR(ty);
        m_ra->m_v2pr->set(vid, vx);
        m_pr2v->set(PR_no(vx), vid);
    }
    vx = m_ru->dupIR(vx);
    IR_dt(vx) = ty;
    return vx;
}


static bool is_range_call(IR const* ir)
{
    if (!ir->is_call()) { return false; }
    //The first parameter is used to record invoke-kind.
    IR const* p = CALL_param_list(ir);
    if (p == NULL || !p->is_const() || !p->is_uint()) {
        return false;
    }
    CHAR const* fname = SYM_name(CALL_idinfo(ir)->get_name());
    ASSERT0(fname);

    if (*fname == '#') {
        //It is an intrinsic call.
        return false;
    }

    INVOKE_KIND ik = (INVOKE_KIND)CONST_int_val(p);
    switch (ik) {
    case INVOKE_UNDEF: UNREACH();
    case INVOKE_VIRTUAL_RANGE:
    case INVOKE_DIRECT_RANGE:
    case INVOKE_SUPER_RANGE:
    case INVOKE_INTERFACE_RANGE:
    case INVOKE_STATIC_RANGE:
        return true;
    case INVOKE_VIRTUAL:
    case INVOKE_SUPER:
    case INVOKE_DIRECT:
    case INVOKE_STATIC:
    case INVOKE_INTERFACE:
        return false;
    default: UNREACH();
    }
    return false;
}


void LTMgr::genRangeCallGroup(IR const* ir)
{
    ASSERT0(is_range_call(ir));
    IR * p;
    for (p = CALL_param_list(ir); p != NULL; p = p->get_next()) {
        if (!p->is_pr()) { continue; }
        break;
    }
    if (p == NULL) { return; }

    LTG * ltg = m_gltm->m_ltgmgr.map_ir2ltg(IR_id(ir));
    if (ltg == NULL) {
        ltg = m_gltm->m_ltgmgr.create();
        m_gltm->m_ltgmgr.set_map_ir2ltg(IR_id(ir), ltg);
    } else {
        ltg->clean();
    }
    ltg->ty = LTG_RANGE_PARAM;

    INT idx = 0;
    for (;p != NULL; p = p->get_next()) {
        ASSERT0(p->is_pr());
        LT * lt = map_pr2lt(PR_no(p));
        ASSERT0(lt);
        LT_ltg_pos(lt) = idx;
        LT_ltg(lt) = ltg;
        ltg->set(idx, lt);
        idx++;
    }
}


void LTMgr::genGroup(LT * first, LT * second)
{
    if (LT_ltg(first) != NULL) {
        ASSERT0(LT_ltg(second));
        ASSERT0(LT_ltg(first) == LT_ltg(second));
        ASSERT0(LT_ltg_pos(first) == 0);
        ASSERT0(LT_ltg_pos(second) == 1);
        return;
    }
    ASSERT0(LT_ltg(second) == NULL);
    LT_ltg(first) = m_gltm->m_ltgmgr.create();
    LT_ltg(second) = LT_ltg(first);
    LT_ltg(first)->ty = LTG_REG_PAIR;

    LT_ltg_pos(first) = 0;
    LT_ltg(first)->set(0, first);

    LT_ltg_pos(second) = 1;
    LT_ltg(first)->set(1, second);
}


void LTMgr::processUseGroupPart(IR const* ir, UINT pos, OUT BitSet & lived_lt)
{
    ASSERT0(ir->is_pr());
    ASSERT0(is_pair(ir));
    LT * lt = map_pr2lt(PR_no(ir));
    ASSERT0(lt);

    bool find = false;
    UINT vreg = m_pr2v->get(PR_no(const_cast<IR*>(ir)), &find);
    IR * sib;
    if (find) {
        vreg++; //ir is always the lowest part of group.
        //sib = genMappedPR(vreg, m_tm->getSimplexTypeEx(D_U32));
        sib = m_v2pr->get(vreg);
        if (sib == NULL) {
            //low part is mapped during dex2ir, but high part may be not appear.
            //So its related IR is NULL.
            sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
        } else {
            sib = m_ru->dupIR(sib);
            IR_dt(sib) = m_tm->getSimplexTypeEx(D_U32);
        }
    } else if (LT_ltg(lt) != NULL) {
        //ir is generated by renaming. lt already has grouped.
        sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
        LT * pair = LT_ltg(lt)->get(1);
        ASSERT0(pair);
        PR_no(sib) = LT_prno(pair);
    } else {
        //ir is generated by renaming.
        sib = m_ru->buildPR(m_tm->getSimplexTypeEx(D_U32));
    }

    LT * siblt = processUsePR(sib, pos, lived_lt);
    ASSERT0(siblt);
    genGroup(lt, siblt);
}


LT * LTMgr::processUsePR(IR const* ir, UINT pos, OUT BitSet & lived_lt)
{
    ASSERT0(ir->is_pr());
    UINT prno = PR_no(ir);
    LT * lt = map_pr2lt(prno);
    if (lt == NULL) {
        lt = newLT(prno);
    }
    LT_range(lt)->bunion(pos);
    if (LT_occ(lt) == NULL) {
        LT_occ(lt) = m_gltm->get_bs_mgr()->create(0);
    }
    LT_occ(lt)->bunion(pos);
    recordPhyRegOcc(lt, pos, lived_lt);
    lived_lt.bunion(LT_uid(lt));
    return lt;
}


//Set lt to be register group. If it belong to global lt, also set the
//flag to the related global lt.
void LTMgr::process_rg(LT * lt)
{
    ASSERT0(lt);
    LT_rg_sz(lt) = RG_PAIR_SZ;
    if (LT_is_global(lt)) {
        GLT * g = m_gltm->m_pr2glt.get(LT_prno(lt));
        ASSERT0(g);

        //If global lt has allocate, then local part need to
        //make sure it must be paired.
        ASSERT(!g->has_allocated() || GLT_rg_sz(g) >= RG_PAIR_SZ,
                ("glt has assigned register, but is not paired."));
        GLT_rg_sz(g) = RG_PAIR_SZ;
    }
}


void LTMgr::processResult(
        IN IR * ir,
        INT pos,
        IN OUT BitSet & lived_lt,
        bool group_part)
{
    ASSERT0(ir->is_stmt());

    //Keep the track of live points at DEF for each lived PR.
    for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
         LT * lt = get_lt(i);
        ASSERT0(lt);
        LT_range(lt)->bunion(pos);
    }
    switch (IR_code(ir)) {
    case IR_ST: break;
    case IR_STPR:
        if (group_part) {
            if (is_pair(ir)) {
                processResultGroupPart(ir, pos, lived_lt);
            }
        } else {
            LT * lt = processResultPR(STPR_no(ir), pos, lived_lt);
            if (is_pair(ir)) {
                process_rg(lt);
            }
        }
        break;
    case IR_IST: break;
    case IR_STARRAY: break;
    case IR_CALL:
    case IR_ICALL:
        if (ir->hasReturnValue()) {
            if (group_part) {
                if (is_pair(ir)) {
                    processResultGroupPart(ir, pos, lived_lt);
                }
            } else {
                LT * lt = processResultPR(CALL_prno(ir), pos, lived_lt);
                if (is_pair(ir)) {
                    process_rg(lt);
                }
            }
        }
        break;
    case IR_GOTO:
    case IR_SWITCH:
    case IR_TRUEBR:
    case IR_FALSEBR:
    case IR_RETURN:
        break;
    case IR_PHI:
        if (group_part) {
            if (is_pair(ir)) {
                processResultGroupPart(ir, pos, lived_lt);
            }
        } else {
            LT * lt = processResultPR(PHI_prno(ir), pos, lived_lt);
            if (is_pair(ir)) {
                process_rg(lt);
            }
        }
        break;
    default: UNREACH();
    }
}


//'group_part': set to true if user is going to scan and handle
//group register info.
void LTMgr::processUse(
        IN IR * ir,
        ConstIRIter & cii,
        INT pos,
        IN OUT BitSet & lived_lt,
        bool group_part)
{
    ASSERT0(ir->is_stmt());

    //Keep the track of live points at USE for each live sr
    for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
        LT * lt = get_lt(i);
        ASSERT0(lt);
        LT_range(lt)->bunion(pos);
    }

    cii.clean();
    for (IR const* k = iterRhsInitC(ir, cii);
         k != NULL; k = iterRhsNextC(cii)) {
        if (!k->is_pr()) { continue; }

        if (group_part) {
            if (is_pair(k)) {
                processUseGroupPart(k, pos, lived_lt);
            }
        } else {
            LT * lt = processUsePR(k, pos, lived_lt);
            if (is_pair(k)) {
                process_rg(lt);
            }
        }
    }

    if (is_range_call(ir)) {
        genRangeCallGroup(ir);
    }
}


bool LTMgr::has_pair_res(IR * ir)
{
    ASSERT0(ir->is_stmt());
    switch(IR_code(ir)) {
    case IR_STPR:
    case IR_CALL:
    case IR_ICALL:
        if (ir->get_dtype_size(m_tm) == PAIR_BYTES) {
            return true;
        }
        break;
    default:;
    }
    return false;
}


IR * LTMgr::genDedicatePR(UINT phy)
{
    UNUSED(phy);
    ASSERT(0, ("Target Dependent Code"));
    return NULL;
}


//Process the function/region exit BB.
void LTMgr::processExitBB(
        IN OUT List<LT*> * liveout_exitbb,
        IN OUT BitSet & lived_lt,
        BitSet const& retval_regset,
        UINT pos)
{
    ASSERT0(liveout_exitbb);
    for (INT phy = retval_regset.get_first();
         phy != -1; phy = retval_regset.get_next(phy)) {
        IR * x = genDedicatePR(phy);
        //TODO: map PR to phy.
        UINT prno = PR_no(x);
        LT * lt = map_pr2lt(prno);
        if (lt == NULL) {
            lt = newLT(prno);
        }
        lived_lt.bunion(prno);
        LT_range(lt)->bunion(pos);
        liveout_exitbb->append_tail(lt);
        m_prdf->setPRToBeLiveout(m_bb, prno);
    }
}


//Process the live in sr.
//'always_consider_glt': true if build local lt for global lt
//        even if glt has not assigned register.
void LTMgr::processLivein(
        OUT BitSet & lived_lt,
        UINT pos,
        bool always_consider_glt)
{
    DefSBitSetCore * livein = m_prdf->get_livein(BB_id(m_bb));
    SEGIter * cur = NULL;
    for (INT i = livein->get_first(&cur);
         i != -1; i = livein->get_next(i, &cur)) {
        GLT * glt = m_gltm->map_pr2glt(i);
        ASSERT0(glt);
        if (glt->has_allocated() || always_consider_glt) {
            LT * lt = map_pr2lt(i);
            if (lt == NULL) {
                lt = newLT(i);
            }
            LT_rg_sz(lt) = MAX(LT_rg_sz(lt), GLT_rg_sz(glt));
            if (LT_rg_sz(lt) > GLT_rg_sz(glt)) {
                //Local part need lt must be paired.
                ASSERT(!glt->has_allocated(),
                        ("glt has assigned register, but is not paired."));
                GLT_rg_sz(glt) = LT_rg_sz(lt);
            }
            LT_is_global(lt) = true; //If lt has created, mark it as global.
            lived_lt.bunion(LT_uid(lt));
        }
    }

    //Keep tracking of live points for each lived PR.
    for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
        LT * lt = get_lt(i);
        ASSERT0(lt);
        LT_range(lt)->bunion(pos);
    }
}


void LTMgr::processLiveout(
        IN OUT BitSet & lived_lt,
        UINT pos,
        bool always_consider_glt)
{
    DefSBitSetCore * liveout = m_prdf->get_liveout(BB_id(m_bb));

    SEGIter * cur = NULL;
    for (INT i = liveout->get_first(&cur);
         i != -1; i = liveout->get_next(i, &cur)) {
        GLT * glt = m_gltm->map_pr2glt(i);
        ASSERT0(glt);
        if (glt->has_allocated() || always_consider_glt) {
            //If global pr has not assign a register,
            //we say it does not interfere local pr.
            LT * lt = map_pr2lt(i);
            if (lt == NULL) {
                lt = newLT(i);
            }
            ASSERT0(LT_rg_sz(lt) <= GLT_rg_sz(glt));
            LT_rg_sz(lt) = GLT_rg_sz(glt);

            LT_is_global(lt) = true; //If lt has created, mark it as global.
            lived_lt.bunion(LT_uid(lt));
        }
    }

    //Keep tracking of live points for each lived PR.
    for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
        LT * lt = get_lt(i);
        ASSERT0(lt);
        LT_range(lt)->bunion(pos);
    }
}


void LTMgr::buildGroup(ConstIRIter & cii)
{
    ASSERT(m_bb != NULL, ("Basic block is NULL"));
    //Add two point for live in exposed use and live out exposed use.
    m_max_lt_len = m_bb->getNumOfIR() * 2 + 2 + get_first_pos();
    BitSet * lived_lt = m_gltm->get_bs_mgr()->create();
    C<IR*> * ct;
    IR * ir = BB_irlist(m_bb).get_tail(&ct);
    UINT pos = m_max_lt_len - 2;
    for (; ir != NULL; ir = BB_irlist(m_bb).get_prev(&ct), pos--) {
        processResult(ir, pos, *lived_lt, true);
        pos--;
        processUse(ir, cii, pos, *lived_lt, true);
    }
    ASSERT0(pos == get_first_pos());
    m_gltm->get_bs_mgr()->free(lived_lt);
}


//'consider_glt': if true to build local life time for unallocated glt.
//'lived_lt': for tmp use.
//'liveout_exitbb_lts': record life times which lived out of the function exit BB.
//'tmp': for tmp use.
//NOTE: If bb is empty, we also need to generate lifetime for live in/out
//    global pr.
void LTMgr::build(
        bool consider_glt,
        List<LT*> * liveout_exitbb_lts,
        ConstIRIter & cii)
{
    ASSERT(m_bb != NULL, ("Basic block is NULL"));
    //Add two point for live in exposed use and live out exposed use.
    m_max_lt_len = m_bb->getNumOfIR() * 2 + 2 + get_first_pos();
    BitSet * lived_lt = m_gltm->get_bs_mgr()->create();
    if (liveout_exitbb_lts != NULL) {
        liveout_exitbb_lts->clean();
    }

    UINT pos = m_max_lt_len - 1;
    processLiveout(*lived_lt, pos, consider_glt);

    #ifdef HAS_COND_DEF
    if (BB_is_exit(m_bb)) {
        //Keep USE point of special register in exit BB.
        //Append lt of each return value of register.
        RegSet const* retval_rs = tmGetRegSetOfReturnValue();
        processExitBB(liveout_exitbb_lts, lived_lt, *retval_rs, pos);
    }
    #endif

    IR * ir;
    C<IR*> * ct;
    for (pos = m_max_lt_len - 2, ir = BB_irlist(m_bb).get_tail(&ct);
         ir != NULL; ir = BB_irlist(m_bb).get_prev(&ct), pos--) {
        m_pos2ir.set(pos, ir);

        //Must-DEF point terminates current life time. But May-DEF is not!
        processResult(ir, pos, *lived_lt, false);
        pos--;
        m_pos2ir.set(pos, ir);
        processUse(ir, cii, pos, *lived_lt, false);
    }

    ASSERT0(pos == get_first_pos());
    processLivein(*lived_lt, pos, consider_glt);

    /*
    //Append the FIRST_POS to complete all remainder life times.
    for (INT i = lived_lt.get_first(); i >= 0; i = lived_lt.get_next(i)) {
         LT * lt = get_lt(i);
        ASSERT0(lt);
        LT_pos(lt)->bunion(pos);
    }
    */

    #ifdef _DEBUG_
    //Life time verification.
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        if (!LT_is_global(lt) && !lt->has_allocated()) {
            /* For the sake of the weak implementation of Code Expansion Phase,
            do not check the existence of the first def-point for local SR,
            even if it does not have in some case. Because, Code Expansion
            Phase might generate redundant SR reference.
            While lt's SR has been assigned a physical register, the life
            time should be able to represent that register.

            ASSERT(LT_pos(lt)->get_first() > get_first_pos(),
                   ("Local life time has not live in point")); */
            ASSERT(LT_range(lt)->get_first() <
                   (INT)(m_max_lt_len - 1),
                   ("Local life time has not live in point"));
        }
    }
    #endif

    //lt in liveout_exitbb_lts may be removed.
    revise_special_lt(liveout_exitbb_lts);
    m_gltm->get_bs_mgr()->free(lived_lt);
}


void LTMgr::clean()
{
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        lt->clean();
    }
    m_pos2ir.clean();
    //Do not clean map between pr from lt.
}


void LTMgr::revise_lt_case_1(LT * lt)
{
    bool is_def;
    INT f = LT_range(lt)->get_first();
    INT first_concrete_occ = lt->get_forward_occ(f, &is_def, f);
    ASSERT(first_concrete_occ > (INT)get_first_pos(),
            ("empty life tiem, have no any occ!"));
    BitSet * tmp = m_gltm->m_bs_mgr.create();
    BitSet * bs = LT_range(lt);
    bs->get_subset_in_range(first_concrete_occ, bs->get_last(), *tmp);
    bs->copy(*tmp);
    //Ignore the vector part of the lt, since we only iter lt via bs.
    m_gltm->m_bs_mgr.free(tmp);
}


void LTMgr::revise_special_lt(List<LT*> * lts)
{
    INT firstpos = get_first_pos();
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        if (!LT_is_global(lt) &&
            LT_range(lt)->get_first() == firstpos) {
            /* Local PR has occurred at LT_FIRST_POS!
            lt might be assigned register already. Apart from that,
            there are followed reasons for the situation at present:
            CASE 1: Local PR that only has USE point. That becasuse Code
                Generation Phase might generate redundant PR reference code,
                or the DEF of local PR is conditional execution. */
            revise_lt_case_1(lt);
        } //end if
    } //end for

    /* Remove lt which live-through exit bb that neither have any
    use/def occurrence nor live-in the exit bb.
    So far, we only found this case in exit bb. Any else?
    For the sake of that, we only check exit bb for speeding up compiling. */
    if (BB_is_exit(m_bb) && lts != NULL) {
        for (LT * lt = lts->get_head(); lt != NULL; lt = lts->get_next()) {
            if (!is_livein(LT_prno(lt)) &&
                (LT_occ(lt) == NULL || LT_occ(lt)->get_elem_count() == 0)) {
                removeLT(lt);
            }
        }
    }
}


void LTMgr::renameUse(IR * ir, LT * l, IR ** newpr)
{
    LTG * gr = LT_ltg(l);
    switch (IR_code(ir)) {
    case IR_STARRAY:
        ASSERT0(((CArray*)ir)->getDimNum() == 1);
        renameUse(STARR_rhs(ir), l, newpr);
        renameUse(ARR_base(ir), l, newpr);
        renameUse(ARR_sub_list(ir), l, newpr);
        break;
    case IR_IST:
        renameUse(IST_base(ir), l, newpr);
        renameUse(IST_rhs(ir), l, newpr);
        break;
    case IR_ST:
        renameUse(ST_rhs(ir), l, newpr);
        break;
    case IR_STPR:
        renameUse(STPR_rhs(ir), l, newpr);
        break;
    case IR_ICALL:
        renameUse(ICALL_callee(ir), l, newpr);
    case IR_CALL:
        {
            IR * next;
            for (IR * p = CALL_param_list(ir); p != NULL; p = next) {
                next = p->get_next();
                if (p->is_pr()) {
                    if (PR_no(p) == LT_prno(l)) {
                        if (*newpr == NULL) {
                            *newpr = m_ru->buildPR(IR_dt(p));
                        }
                        IR * newp = m_ru->dupIR(*newpr);
                        IR_dt(newp) = IR_dt(p);
                        replace(&CALL_param_list(ir), p, newp);
                        IR_parent(newp) = ir;
                        m_ru->freeIR(p);
                    } else if (gr != NULL &&
                               gr->is_member(PR_no(p))) {
                        if (*newpr == NULL) {
                            *newpr = m_ru->buildPR(IR_dt(p));
                        }
                    }
                }
            }
        }
        break;
    case IR_TRUEBR:
    case IR_FALSEBR:
        renameUse(BR_det(ir), l, newpr);
        break;
    case IR_SWITCH:
        renameUse(SWITCH_vexp(ir), l, newpr);
        break;
    case IR_RETURN:
        {
            ASSERT0(cnt_list(RET_exp(ir)) <= 1);
            IR * rv = RET_exp(ir);
            if (rv != NULL && rv->is_pr()) {
                if (PR_no(rv) == LT_prno(l)) {
                    if (*newpr == NULL) {
                        *newpr = m_ru->buildPR(IR_dt(rv));
                    }
                    IR * x = m_ru->dupIR(*newpr);
                    IR_dt(x) = IR_dt(rv);
                    RET_exp(ir) = x;
                    IR_parent(x) = ir;
                    m_ru->freeIR(rv);
                } else if (gr != NULL &&
                           gr->is_member(PR_no(rv))) {
                    if (*newpr == NULL) {
                        *newpr = m_ru->buildPR(IR_dt(rv));
                    }
                }
            }
        }
        break;
    case IR_REGION:
        UNREACH();
        break;
    case IR_GOTO: break;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_REM:
    case IR_MOD:
    case IR_LAND: //logical and &&
    case IR_LOR: //logical or ||
    case IR_BAND: //inclusive and &
    case IR_BOR: //inclusive or |
    case IR_XOR: //exclusive or
    case IR_LT:
    case IR_LE:
    case IR_GT:
    case IR_GE:
    case IR_EQ: //==
    case IR_NE: //!=
    case IR_ASR:
    case IR_LSR:
    case IR_LSL:
        renameUse(BIN_opnd0(ir), l, newpr);
        renameUse(BIN_opnd1(ir), l, newpr);
        break;
    case IR_BNOT: //bitwise not
    case IR_LNOT: //logical not
    case IR_NEG: //negative
        renameUse(UNA_opnd0(ir), l, newpr);
        break;
    case IR_CVT: //type convertion
        renameUse(CVT_exp(ir), l, newpr);
        break;
    case IR_LDA:
    case IR_ID:
    case IR_LD:
        break;
    case IR_PR:
        {
            IR * p = IR_parent(ir);
            ASSERT0(p); //only process PR here for that has a parent.
            for (INT i = 0; i < IR_MAX_KID_NUM(p); i++) {
                IR * t = p->get_kid(i);
                if (t == NULL || !t->is_pr()) { continue; }
                if (PR_no(t) == LT_prno(l)) {
                    if (*newpr == NULL) {
                        *newpr = m_ru->buildPR(IR_dt(t));
                    }
                    IR * x = m_ru->dupIR(*newpr);
                    IR_dt(x) = IR_dt(t);
                    p->set_kid(i, x);
                    IR_parent(x) = p;
                    m_ru->freeIR(t);
                } else if (gr != NULL && gr->is_member(PR_no(t))) {
                    if (*newpr == NULL) {
                        *newpr = m_ru->buildPR(IR_dt(t));
                    }
                }
            }
        }
        break;
    case IR_ARRAY:
        ASSERT0(((CArray*)ir)->getDimNum() == 1);
        renameUse(ARR_base(ir), l, newpr);
        renameUse(ARR_sub_list(ir), l, newpr);
        break;
    case IR_ILD:
        renameUse(ILD_base(ir), l, newpr);
        break;
    case IR_CONST: break;
    default: UNREACH();
    }
}


//Rename lifetime's PR with new PR.
void LTMgr::renameLT(LT * l, IR ** newpr)
{
    ASSERT0(l && newpr);
    BitSet * occ = LT_occ(l);
    if (occ == NULL) { return; }
    for (INT i = occ->get_first(); i >= 0; i = occ->get_next(i)) {
        IR * ir = m_pos2ir.get(i);
        ASSERT0(ir);
        if (l->is_def(i)) {
            switch (IR_code(ir)) {
            case IR_STPR:
                {
                    UINT prno = ir->get_prno();
                    if (prno == LT_prno(l)) {
                        if (*newpr == NULL) {
                            //Generate new PR no.
                            *newpr = m_ru->buildPR(IR_dt(ir));
                        }
                        ir->set_prno(PR_no(*newpr));
                    } else {
                        //l is a member of group.
                        ASSERT0(LT_ltg(l) != NULL);
                        ASSERT0(LT_ltg(l)->is_member(prno));
                        if (*newpr == NULL) {
                            *newpr = m_ru->buildPR(IR_dt(ir));
                        }
                    }
                }
                break;
            case IR_CALL:
            case IR_ICALL:
                    if (CALL_prno(ir) == LT_prno(l)) {
                        if (*newpr == NULL) {
                            *newpr = m_ru->buildPR(IR_dt(ir));
                        }
                        CALL_prno(ir) = PR_no(*newpr);
                    } else {
                        //l is a member of group.
                        ASSERT0(LT_ltg(l) != NULL);
                        ASSERT0(LT_ltg(l)->is_member(CALL_prno(ir)));
                        if (*newpr == NULL) {
                            *newpr = m_ru->buildPR(IR_dt(ir));
                        }
                    }
                break;
            default: ASSERT(0, ("no def to PR"));
            }
        } else {
            renameUse(ir, l, newpr);
        }
    }
    if (*newpr != NULL) {
        LT_prno(l) = PR_no(*newpr);
        m_prno2lt.setAlways(LT_prno(l), l);
    }
}


//If local PR is same as global PR, rename local PR.
//If there are multiple local lifetime corresponded to same PR, rename them.
void LTMgr::rename(TMap<UINT, LT*> & prno2lt, BitSet & met)
{
    prno2lt.clean(); //for tmp use
    for (INT i = 1; i <= m_lt_vec.get_last_idx(); i++) {
        LT * l = m_lt_vec.get(i);
        if (l == NULL) { continue; }
        if (!LT_is_global(l)) {
            if (m_gltm->m_pr2glt.get(LT_prno(l)) != NULL) {
                //local lifetime has same PR with global lifetime.
                IR * newpr = NULL;
                renameLT(l, &newpr);
                if (newpr != NULL) {
                    met.bunion(PR_no(newpr));
                }
                continue;
            }
            if (met.is_contain(LT_prno(l))) {
                //local lifetime has same PR with local lifetime in other bb.
                IR * newpr = NULL;
                renameLT(l, &newpr);
                if (newpr != NULL) {
                    met.bunion(PR_no(newpr));
                }
                continue;
            }
        }

        LT * prior = prno2lt.get(LT_prno(l));
        if (prior == NULL) {
            prno2lt.set(LT_prno(l), l);
            met.bunion(LT_prno(l));
        } else if (prior != l) {
            //Do not rename global lifetime in LTMgr.
            IR * newpr = NULL;
            if (LT_is_global(l) && !LT_is_global(prior)) {
                prno2lt.setAlways(LT_prno(l), l);
                renameLT(prior, &newpr);
            } else {
                ASSERT(!LT_is_global(l) || !LT_is_global(prior),
                        ("glt wit same pr"));
                renameLT(l, &newpr);
            }
            if (newpr != NULL) {
                met.bunion(PR_no(newpr));
            }
            m_ru->freeIR(newpr);
        }
    }
}


void LTMgr::removeLT(LT * lt)
{
    m_lt_vec.set(LT_uid(lt), NULL);
    m_prno2lt.setAlways(LT_prno(lt), NULL);
    m_gltm->get_bs_mgr()->free(LT_range(lt));
    m_gltm->get_bs_mgr()->free(LT_occ(lt));
    LT_range(lt) = NULL;
    LT_occ(lt) = NULL;
    m_ig.removeVertex(LT_uid(lt));
}


void LTMgr::dump_allocated(FILE * h, BitSet & visited)
{
    //Print local life time.
    //Position start from 0, end at maxlen-1.
    fprintf(h, "\nPOS:");
    for (INT i = 1; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        visited.bunion(LT_uid(lt));
        if (LT_is_global(lt)) {
            fprintf(h, "\n GLT(%3d):", LT_uid(lt));
        } else {
            fprintf(h, "\n  LT(%3d):", LT_uid(lt));
        }

        //Collects position info.
        CHAR * pos_marker = (CHAR*)malloc(m_max_lt_len);
        memset(pos_marker, 0, sizeof(CHAR) * m_max_lt_len);
        for (INT j = LT_range(lt)->get_first();
             j >= 0; j = LT_range(lt)->get_next(j)) {
            ASSERT0(j < (INT)m_max_lt_len);
            pos_marker[j] = 1;
        }

        //Dump life time.
        for (UINT k = 0; k < m_max_lt_len; k++) {
            if (pos_marker[k] == 0) {
                fprintf(h, "   ,");
            } else {
                fprintf(h, "%3d,", k);
            }
        }
        free(pos_marker);

        //Dump prno, phy.
        fprintf(h, "    [pr%d]", LT_prno(lt));
        if (lt->has_allocated()) {
            fprintf(h, "(");
            for (INT z = 0; z < LT_rg_sz(lt); z++) {
                fprintf(h, "v%d", LT_phy(lt) + z);
                if (z != (LT_rg_sz(lt) - 1)) {
                    fprintf(h, ",");
                }
            }
            fprintf(h, ")");
        } else {
            fprintf(h, "(");
            for (INT z = 0; z < LT_rg_sz(lt); z++) {
                fprintf(h, "-");
                if (z != (LT_rg_sz(lt) - 1)) {
                    fprintf(h, ",");
                }
            }
            fprintf(h, ")");
        }

        if (LT_ltg(lt) != NULL) {
            fprintf(h, ",");
            LTG * ltg = LT_ltg(lt);
            switch (ltg->ty) {
            case LTG_RANGE_PARAM:
                fprintf(h, "rg"); break;
            case LTG_REG_PAIR:
                fprintf(h, "pg"); break;
            default: UNREACH();
            }

            //Dump ltg's prno, ltid, phy.
            fprintf(h, "<");
            for (INT i = 0; i <= ltg->get_last_idx(); i++) {
                LT * l = ltg->get(i);
                ASSERT0(l);
                fprintf(h, "pr%d(lt%d)", LT_prno(l), LT_uid(l));
                if (l->has_allocated()) {
                    INT sz = LT_rg_sz(l);

                    fprintf(h, "(");
                    for (INT z = 0; z < sz; z++) {
                        fprintf(h, "v%d", LT_phy(l) + z);
                        if (z != (LT_rg_sz(l) - 1)) {
                            fprintf(h, ",");
                        }
                    }
                    fprintf(h, ")");
                }
                fprintf(h, " ");
            }
            fprintf(h, ">");
            //
        }
    }

    fprintf(h, "\nDESC:");
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        if (LT_is_global(lt)) {
            fprintf(h, "\n GLT(%3d):", LT_uid(lt));
        } else {
            fprintf(h, "\n  LT(%3d):", LT_uid(lt));
        }

        //Dump occurrences.
        INT start = LT_range(lt)->get_first();
        INT end = LT_range(lt)->get_last();
        for (INT j = get_first_pos(); j < (INT)m_max_lt_len; j++) {
            if (LT_occ(lt) != NULL && LT_occ(lt)->is_contain(j)) {
                if (lt->is_def(j)) {
                    fprintf(h, "DEF,");
                } else {
                    fprintf(h, "USE,");
                }
            } else {
                if (j >= start && j <= end) {
                    fprintf(h, "   ,");
                } else {
                    fprintf(h, "    ");
                }
            }
        }

        //Dump prno, phy.
        fprintf(h, "    [pr%d]", LT_prno(lt));
        if (lt->has_allocated()) {
            fprintf(h, "(");
            for (INT z = 0; z < LT_rg_sz(lt); z++) {
                fprintf(h, "v%d", LT_phy(lt) + z);
                if (z != LT_rg_sz(lt) - 1) {
                    fprintf(h, ",");
                }
            }
            fprintf(h, ")");
        } else {
            fprintf(h, "(");
            for (INT z = 0; z < LT_rg_sz(lt); z++) {
                fprintf(h, "-");
                if (z != (LT_rg_sz(lt) - 1)) {
                    fprintf(h, ",");
                }
            }
            fprintf(h, ")");
        }

        if (LT_ltg(lt) != NULL) {
            fprintf(h, ",");
            LTG * ltg = LT_ltg(lt);
            switch (ltg->ty) {
            case LTG_RANGE_PARAM:
                fprintf(h, "rg"); break;
            case LTG_REG_PAIR:
                fprintf(h, "pg"); break;
            default: UNREACH();
            }

            //Dump ltg's prno, ltid, phy.
            fprintf(h, "<");
            for (INT i = 0; i <= ltg->get_last_idx(); i++) {
                LT * l = ltg->get(i);
                ASSERT0(l);
                fprintf(h, "pr%d(lt%d)", LT_prno(l), LT_uid(l));
                if (l->has_allocated()) {
                    INT sz = LT_rg_sz(l);

                    fprintf(h, "(");
                    for (INT z = 0; z < sz; z++) {
                        fprintf(h, "v%d", LT_phy(l) + z);
                        if (z != (LT_rg_sz(l) - 1)) {
                            fprintf(h, ",");
                        }
                    }
                    fprintf(h, ")");
                }
                fprintf(h, " ");
            }
            fprintf(h, ">");
            //
        }
    }
}


void LTMgr::dump_unallocated(FILE * h, BitSet & visited)
{
    //Print unallocated life time.
    bool doit = false;
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        if (!visited.is_contain(LT_uid(lt))) {
            doit = true;
            break;
        }
    }
    if (!doit) { return; }

    fprintf(h, "\nUnallocated:");
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        if (visited.is_contain(LT_uid(lt))) { continue; }
        fprintf(h, "\nPOS:");
        fprintf(h, "\n  LT(%3d):", LT_uid(lt));

        //Collects position info.
        CHAR * pos_marker = (CHAR*)malloc(m_max_lt_len);
        memset(pos_marker, 0, sizeof(CHAR) * m_max_lt_len);
        for (INT j = LT_range(lt)->get_first();
             j >= 0; j = LT_range(lt)->get_next(j)) {
            ASSERT0(j < (INT)m_max_lt_len);
            pos_marker[j] = 1;
        }

        //Position start from 0, end at maxlen-1.
        for (UINT k = 0; k < m_max_lt_len; k++) {
            if (pos_marker[k] == 0) {
                fprintf(h, "   ,");
            } else {
                fprintf(h, "%3d,", k);
            }
        }
        free(pos_marker);
        fprintf(h, "    [pr%d]", LT_prno(lt));

        //Collects position info
        if (LT_occ(lt) == NULL) { continue; }
        INT last_idx = LT_occ(lt)->get_last();
        UNUSED(last_idx);
        ASSERT(last_idx == -1 || last_idx < (INT)m_max_lt_len,
                ("Depiction of life time long than the finial position"));

        fprintf(h, "\nDESC:");
        INT start = LT_occ(lt)->get_first();
        INT end = LT_occ(lt)->get_last();
        for (INT j = get_first_pos(); j < (INT)m_max_lt_len; j++) {
            if (LT_occ(lt) != NULL && LT_occ(lt)->is_contain(j)) {
                if (lt->is_def(j)) {
                    fprintf(h, "DEF,");
                } else {
                    fprintf(h, "USE,");
                }
            } else {
                if (j >= start && j <= end) {
                    fprintf(h, "   ,");
                } else {
                    fprintf(h, "    ");
                }
            }
        }
        fprintf(h, "    [pr%d]", LT_prno(lt));
    } //end for
}


void LTMgr::dump()
{
    if (g_tfile == NULL) return;
    fprintf(g_tfile, "\n--- BB%d Local Life Time ---", m_bb->id);

    //Print live-in PR.
    fprintf(g_tfile, "\nlivein:");
    DefSBitSetCore * livein = m_prdf->get_livein(BB_id(m_bb));
    SEGIter * cur = NULL;
    for (INT i = livein->get_first(&cur);
         i != -1; i = livein->get_next(i, &cur)) {
        fprintf(g_tfile, "pr%d, ", i);
    }

    //Print live-out PR.
    fprintf(g_tfile, "\nliveout:");
    DefSBitSetCore * liveout = m_prdf->get_liveout(BB_id(m_bb));
    for (INT i = liveout->get_first(&cur);
         i != -1; i = liveout->get_next(i, &cur)) {
        fprintf(g_tfile, "pr%d, ", i);
    }

    //Print local life times base info.
    UINT c = 0;
    for (INT i = 0; i <= m_lt_vec.get_last_idx(); i++) {
        LT * lt = m_lt_vec.get(i);
        if (lt == NULL) { continue; }
        c = MAX(c, LT_uid(lt));
        //fprintf(g_tfile, "\n\tLT(%d):", LT_uid(lt));
        //fprintf(g_tfile, "[pr%d]:", LT_prno(lt));
    }

    BitSet visited(c + 1);
    dump_allocated(g_tfile, visited);
    //dump_unallocated(g_tfile, visited);
    fflush(g_tfile);
}
//END LTMgr


//
//START BBRA
//
BBRA::BBRA(IRBB * bb, RA * ra)
{
    ASSERT0(ra && bb);
    m_bb = bb;
    m_ra = ra;
    m_rsc = ra->get_rsc();
    m_gltm = ra->get_gltm();
    m_ru = ra->m_ru;
    ASSERT0(m_gltm);
    m_ltm = m_gltm->map_bb2ltm(bb);
    ASSERT0(m_ltm);
    m_ig = m_ltm->get_ig();
    m_tmp_lts = NULL;
    m_tmp_lts2 = NULL;
    m_tmp_uints = NULL;
}


/* Compute priority list and sort life times with descending order of priorities.

'lts': list of LT.
'prios': list of LT, which elements are sorted in descending order of priority.

We use some heuristics factors to evaluate the
priorities of each of life times:
    1. Life time in critical path will be put in higher priority.
    2. Life time whose symbol register referenced in
        high density will have higher priority.
    3. Life time whose usable registers are fewer, the priority is higher.

    TO BE ESTIMATED:
        Longer life time has higher priority. */
void BBRA::buildPrioList(IN List<LT*> const& lts, OUT List<LT*> & prios)
{
    C<LT*> * ct;
    for (LT * lt = lts.get_head(&ct); lt != NULL; lt = lts.get_next(&ct)) {
        LT_priority(lt) = computePrio(lt);

        //Search for appropriate position to place.
        LT * t;
        C<LT*> * ct2;
        for (t = prios.get_head(&ct2); t != NULL; t = prios.get_next(&ct2)) {
            if (LT_priority(t) < LT_priority(lt)) {
                break;
            }
        }
        if (t == NULL) {
            prios.append_tail(lt);
        } else {
            prios.insert_before(lt, ct2);
        }
    }
}


float BBRA::computePrio(LT const* lt)
{
    ASSERT0(!LT_is_global(lt));
    if (LT_range(lt) == NULL) return 0.0;
    BitSet const* occ = LT_occ(lt);
    ASSERT0(occ);
    float prio = (float)occ->get_elem_count();
    BitSet const* usable = LT_usable(lt);
    if (usable != NULL) {
        if (m_rsc->get_4() == usable) {
            prio += 1000;
        } else if (m_rsc->get_8() == usable) {
            prio += 800;
        } else if (m_rsc->get_16() == usable) {
            prio += 400;
        }
    }

    if (LT_rg_sz(lt) > 1) {
        prio *= (float)(LT_rg_sz(lt) * 2);
    }

    /* occ in truebr/falsebr/switch/goto should has highest priority
    e.g: br pr1,pr2
    If there is not avaiable register for parameters, we
    need to spill other LT. Here we can not spill live through
    global lt, because a reload need to be inserted at the end
    of BB. That violates the constrain of BB.
    In order to avoid the rarely situation, lt with branch should get a
    highest priority. */
    if (lt->has_branch(m_ltm)) {
        prio *= 1000;
    }
    return prio;
}


//'prio' list should be sorted in descending order.
void BBRA::allocPrioList(OUT List<LT*> & prios, List<UINT> & nis)
{
    C<LT*> * ct, * next_ct;
    for (prios.get_head(&ct), next_ct = ct; ct != NULL; ct = next_ct) {
        LT * lt = ct->val();
        prios.get_next(&next_ct);
        ASSERT0(!lt->has_allocated());
        if (!assignRegister(lt, nis)) {
            continue;
        }
        ASSERT0(lt->has_allocated());
        m_ra->updateLTMaxReg(lt);
        prios.remove(ct);
    }
}


/* Return true if allocation was successful, otherwise return false.
When register assigned to 'g', it must be deducted from
the usable_register_set of all its neighbors.

'unusable': for tmp use.
'ig': interference graph. */
bool BBRA::assignRegister(LT * l, List<UINT> & nis)
{
    ASSERT0(!LT_is_global(l));
    ASSERT0(!l->has_allocated());
    BitSet const* usable = LT_usable(l);
    if (usable == NULL) { return false; }
    BitSet * unusable = m_gltm->get_bs_mgr()->create();

    //Deduct the used register by neighbors.
    nis.clean();
    bool on = m_ig->get_neighbor_list(nis, LT_uid(l));
    CK_USE(on);
    UINT n = nis.get_elem_count();
    for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
        LT const* ni = m_ltm->get_lt(i);
        ASSERT0(ni);
        if (!ni->has_allocated()) { continue; }
        unusable->bunion(LT_phy(ni));
        if (LT_rg_sz(ni) != 1) {
            ASSERT0(LT_rg_sz(ni) == RG_PAIR_SZ);
            unusable->bunion(LT_phy(ni) + 1);
        }
    }

    //Select preference register.
    UINT pref = LT_prefer_reg(l);
    if (pref != REG_UNDEF && !unusable->is_contain(pref)) {
        if (LT_rg_sz(l) > 1) {
            ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
            if (!unusable->is_contain(pref + 1)) {
                LT_phy(l) = (USHORT)pref;
                m_gltm->get_bs_mgr()->free(unusable);
                ASSERT0(usable->is_contain(pref));
                ASSERT0(!m_ra->is_cross_param(pref, LT_rg_sz(l)));
                return true;
            }
        } else {
            LT_phy(l) = (USHORT)pref;
            m_gltm->get_bs_mgr()->free(unusable);
            ASSERT0(usable->is_contain(pref));
            return true;
        }
    }

    //Avoid allocating the registers which
    //are neighbors the most preferable or anticipated.
    n = nis.get_elem_count();
    for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
        LT const* ni = m_ltm->get_lt(i);
        ASSERT0(ni);
        if (ni->has_allocated()) { continue; }
        //Avoid select the reg which ni preferable.
        UINT r = LT_prefer_reg(ni);
        if (r != REG_UNDEF) {
            unusable->bunion(r);
            if (LT_rg_sz(ni) != 1) {
                ASSERT0(LT_rg_sz(ni) == RG_PAIR_SZ);
                unusable->bunion(r + 1);
            }
        }
    }

    /*
    UINT reg = REG_UNDEF;
    BitSet const* anti = m_ltm->get_anti_regs(l, false);
    if (anti != NULL) {
        //Allocate register that 'g' anticipated.
        for (INT r = anti->get_first(); r >= 0; r = anti->get_next(r)) {
            if (usable->is_contain(r)) {
                LT_phy(l) = r;
                return true;
            }
        }
    }
    */

    //Fine, you can allocate any register for it except the unusables.
    //Preferred the minmum.
    INT m = usable->get_last();
    bool succ = false;
    for (INT i = FIRST_PHY_REG; i <= m; i++) {
        if (!unusable->is_contain(i)) {
            if (LT_rg_sz(l) > 1) {
                ASSERT(LT_rg_sz(l) == RG_PAIR_SZ, ("to support more size"));
                if (!unusable->is_contain(i + 1) &&
                    !m_ra->is_cross_param(i, LT_rg_sz(l))) {
                    LT_phy(l) = (USHORT)i;
                    succ = true;
                    break;
                }
            } else {
                LT_phy(l) = (USHORT)i;
                succ = true;
                break;
            }
        }
    }
    m_gltm->get_bs_mgr()->free(unusable);
    ASSERT0(!succ || usable->is_contain(LT_phy(l)));
    return succ;
}


bool BBRA::canBeSplit(LT const* lt) const
{
    UINT c = LT_range(lt)->get_elem_count();
    if (c == 1) { return false; }
    if (c == 2) {
        INT pos1 = LT_occ(lt)->get_first();
        INT pos2 = LT_occ(lt)->get_next(pos1);
        ASSERT0(pos1 != -1 && pos2 != -1);
        ASSERT(LT_occ(lt)->get_next(pos2) == -1, ("More than 2"));
        if (pos2 == pos1 + 1) {
            //We canot benefit from spilling this lifetime.
            return false;
        }
    }
    return true;
}


//Return true if there is hole in lifetime of 'lt',
//and 'startpos', 'endpos' represented the start and end position of hole.
bool BBRA::get_max_hole(OUT INT * startpos, OUT INT * endpos, LT const* lt)
{
    *startpos = 0;
    *endpos = 0;

    INT maxlen = 0;
    INT next_i = -1;
    INT first = m_ltm->get_first_pos();
    INT last = m_ltm->get_last_pos();
    INT range_last = LT_range(lt)->get_last();
    for (INT i = LT_range(lt)->get_first(), start = i;
         i >= 0 && i <= range_last; i = next_i) {

        next_i = i + 1;
        if (!LT_range(lt)->is_contain(i)) {
            next_i = LT_range(lt)->get_next(i); //'next_i' may be -1
            start = next_i;
            continue;
        }

        if ((i == first && LT_range(lt)->is_contain(first)) || //life time live in.
            (i == last && LT_range(lt)->is_contain(last)) || //life time live out.
            (LT_occ(lt) != NULL && LT_occ(lt)->is_contain(i))) {
            if (i > start) {
                if (maxlen < (i - start)) {
                    maxlen = i - start;
                    *startpos = start;
                    *endpos = i;
                }
            }
            start = i;
        }
    }

    if (*startpos != *endpos) {
        return true;
    }
    return false;
}


//Calculate the number of lifetimes which only living in the 'hole'.
//Only compute the longest hole for each of life times.
void BBRA::computeLTResideInHole(OUT List<LT*> & reside_in, LT const* lt)
{
    INT hole_startpos, hole_endpos;
    get_max_hole(&hole_startpos, &hole_endpos, lt);

    BitSet * hole = m_gltm->get_bs_mgr()->create();
    LT_range(lt)->get_subset_in_range(hole_startpos, hole_endpos, *hole);

    Vector<LT*> * ltvec = m_ltm->get_lt_vec();
    for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
        LT * l = ltvec->get(i);
        if (l == NULL) { continue; }
        if (l == lt) { continue; }
        if (hole->is_contained_in_range(LT_range(l)->get_first(),
                                        LT_range(l)->get_last(),
                                        true)) {
            reside_in.append_tail(l);
        }
    }
    m_gltm->get_bs_mgr()->free(hole);
}


//Return true if l is global live through lifetime.
//There is not any occ in BB for the lifetime.
bool BBRA::is_live_through(LT const* l) const
{
    if (!LT_is_global(l)) { return false; }
    if (LT_occ(l) == NULL || LT_occ(l)->is_empty()) {
        return true;
    }
    return false;
}


//Return true if the phy of cand satisified lt's constrain.
bool BBRA::isSatisfiedConstrain(LT * lt, LT * cand)
{
    if (!cand->has_allocated()) { return false; }
    ASSERT0(LT_usable(lt));
    return LT_usable(lt)->is_contain(LT_phy(cand));
}


/* Determining which one should be spilled.
    Computing spill cost:
    The quotient is bigger, the spill cost is less,
    is also the one we expect to spill.
    <cost = number of uncolored neighbors of 'ni' / 'ni's priority>

Return the spilling candidate life time selected.
And 'has_hole' will be set to TRUE, if we could find a
lifetime hole which contained several
shorter lifetimes in it. */
LT * BBRA::computeSplitCand(LT * lt, bool & has_hole, List<LT*> * tmp,
                           List<LT*> * tmp2)
{
    ASSERT0(tmp && tmp2);
    LT * best = NULL, * better = NULL;
    has_hole = false;

    //Calculates the benefits if 'ni' is deal with as 'action' descriptive.
    List<LT*> * ni_list = tmp; //neighbor list of 'lt'

    //Inspecting all of neighbors of 'lt' even itself.
    m_ig->get_neighbor(*ni_list, lt);
    //if (try_self) {
    //    ni_list.append_head(lt); //May be we should spill 'lt' itself as well.
    //}

    double * ni_cost =
        (double*)ALLOCA(ni_list->get_elem_count() * sizeof(double));

    //1. Computing the cost of each of neighbours in terms of life time
    //     priority and the benefit when we split the
    //   life time.
    INT i = 0;
    List<LT*> * ni_ni_list = tmp2; //neighbor list of 'ni'
    for (LT * ni = ni_list->get_head();
         ni != NULL; ni = ni_list->get_next(), i++) {
        if (!isSatisfiedConstrain(lt, ni)) { continue; }
        if (is_live_through(ni)) {
            has_hole = true;
            return ni;
        }
        ni_cost[i] = 0.0;
        //if (!canBeSplit(ni)) { continue; }

        m_ig->get_neighbor(*ni_ni_list, ni);
        UINT uncolored_nini = 0;
        for (LT * nini = ni_ni_list->get_head();
             nini != NULL; nini = ni_ni_list->get_next()) {
            if (!nini->has_allocated()) {
                uncolored_nini++;
            }
        }

        double c = (uncolored_nini + EPSILON) / (LT_priority(ni) + EPSILON);
        ni_cost[i] = c;
    }

    double const* nic = ni_cost;

    //Selecting policy.
    //2. Choosing the best one as split-candidate that
    //     constains the most life times
    //   which unallocated register till now.
    INT most = 0, most_idx = -1;
    INT minor = 0, minor_idx = -1;
    i = 0;
    List<LT*> * residein_lts = tmp2;
    for (LT * ni = ni_list->get_head();
         ni != NULL; ni = ni_list->get_next(), i++) {
        if (!isSatisfiedConstrain(lt, ni)) { continue; }
        if (!canBeSplit(ni)) { continue; }

        residein_lts->clean();
        computeLTResideInHole(*residein_lts, ni);

        //Find lifetimes which did not assign register yet.
        INT lt_num = 0;
        for (LT * x = residein_lts->get_head();
             x != NULL; x = residein_lts->get_next()) {
            if (!x->has_allocated()) {
                lt_num++;
            }
        }

        if (most < lt_num ||
            (most_idx != -1 && most == lt_num &&
             nic[most_idx] < nic[i])) {
            most_idx = i;
            best = ni;
            most = lt_num;
        }

        //Find the inferior split candidate.
        //We say it is the split-cand if the number of life
        //times which residing in its hole to be the most.
        if (minor < (INT)residein_lts->get_elem_count() ||
            (minor_idx != -1 &&
             minor == (INT)residein_lts->get_elem_count() &&
             nic[minor_idx] < nic[i])) {
            minor_idx = i;
            better = ni;
            minor = residein_lts->get_elem_count();
        }
    }
    if (best == NULL && better != NULL) { //The alternative choose.
        best = better;
    }
    if (best != NULL) {
        //Simply select the lift time with largest hole.
        has_hole = true;
        return best;
    }

    //3. If the first step failed, we choose candidate in terms of
    //     life time priority and the benefit when we split the candidate.
    i = 0;

    //We can obtain some benefits via the adjustment of 'action'.
    double maximal_cost = 0.0;
    ni_ni_list = tmp2; //neighbor list of 'ni'
    for (LT * ni = ni_list->get_head(); ni != NULL; ni = ni_list->get_next(), i++) {
        if (!canBeSplit(ni)) { continue; }
        m_ig->get_neighbor(*ni_ni_list, ni);
        UINT uncolored_nini = 0;
        for (LT * nini = ni_ni_list->get_head();
             nini != NULL; nini = ni_ni_list->get_next()) {
            if (!nini->has_allocated()) {
                uncolored_nini++;
            }
        }

        double c = nic[i];
        if (best == NULL || maximal_cost < c) {
            if (ni != lt) {
                //Avoid the followed case:
                //same start pos, or same end pos.
                if ((LT_range(ni)->get_first() == LT_range(lt)->get_first()) ||
                    (LT_range(ni)->get_last() == LT_range(lt)->get_last())) {
                    continue;
                }
            }
            maximal_cost = c;
            best = ni;
        }
    }
    ASSERT(best != NULL, ("Not any spill candidate."));
    return best;
}


//Return true if there is hole in lifetime of 'owner' that
//'inner' can be lived in, and 'startpos','endpos' represented the hole.
bool BBRA::find_hole(
        OUT INT & startpos,
        OUT INT & endpos,
        LT const* owner,
        LT const* inner)
{
    startpos = 0;
    endpos = 0;

    BitSet * owner_range = LT_range(owner);
    INT lastbit = owner_range->get_last();
    INT firstpos = m_ltm->get_first_pos();
    INT lastpos = m_ltm->get_last_pos();
    IR const* lastir = BB_last_ir(m_bb);
    if (lastir != NULL &&
        (lastir->is_cond_br() ||
         lastir->is_multicond_br() ||
         lastir->is_uncond_br())) {
        lastpos -= 2;
    }

    BitSet * owner_occ = LT_occ(owner);
    INT next_i;
    for (INT i = owner_range->get_first(), start = i;
         i >= 0 && i <= lastbit; i = next_i) {
        next_i = i + 1;

        //Current pos is not a point of life time.
        if (!owner_range->is_contain(i)) {
            next_i = owner_range->get_next(i); //'next_i' may be -1
            start = next_i;
            continue;
        }

        //owner is livein lt.
        if ((i == firstpos && owner_range->is_contain(firstpos)) ||
            ((i == (INT)lastpos) && owner_range->is_contain(lastpos)) ||
            (owner_occ != NULL && owner_occ->is_contain(i))) {

            if (i > start) {
                if (LT_range(inner)->is_contained_in_range(start, i, true)) {
                    startpos = start;
                    endpos = i;
                    return true;
                }
            }
            start = i;
        }//end if
    }//end for
    return false;
}


/* Given two position within lifetime 'lt', tring to choose the most
appropriate split point and inserting the spill/reload code at them.
'is_pos1_spill': if true indicate that a spilling is needed at pos1,
    otherwise to insert a reload.
'is_pos2_spill': if true indicate that a spilling is needed at pos2,
    otherwise to insert a reload.
'lt': split candidate, may be global and local lifetime.

e.g: Given pos1, pos2, both of them are USE.
    We need to find the DEF to insert the spill code. And choosing
    the best USE between 'pos1' and 'pos2' to insert reload code.
    While both positions are useless, we do not insert any code in
    those positions, and set 'pos1' and 'pos2' to -1. */
void BBRA::selectReasonableSplitPos(
        OUT INT & pos1,
        OUT INT & pos2,
        OUT bool & is_pos1_spill,
        OUT bool & is_pos2_spill,
        LT * lt)
{
    ASSERT(lt && pos1 >= 0 && pos2 > 0 && pos1 < pos2, ("Illegal hole"));
    INT p1 = pos1, p2 = pos2;
    bool is_p1_def = false, is_p2_def = false;

    INT firstpos = m_ltm->get_first_pos();
    INT lastpos = m_ltm->get_last_pos();
    BitSet * occ = LT_occ(lt);
    if ((pos1 == firstpos && pos2 == lastpos) || occ == NULL) {
        //live through lt.
        is_pos1_spill = true; //need spill
        is_pos2_spill = false; //need reload
        return;
    }

    //Compute the status the pos shows.
    bool proc = true;
    while (proc) {
        if (p1 == firstpos) {
            is_p1_def = true;
            break;
        } else {
            if (!occ->is_contain(p1)) {
                /* CASE: image.c:copy_rdopt_data:BB1
                     live in and out gsr: GSR277
                     first pos:0
                     last pos:83
                     LT(33): 0,,...,28,29,30...83
                There is a invalid region in between 0~28,and in actually,
                position 28 has not any PI corresponding to!
                Since GSR238(a7) also allocate the same register as
                GSR277 and it has a def at position 28.
                LTMgr handled the situation conservatively.

                But I thought this is a GRA bug:
                    See BB5 for more details:
                        SR284, SR282, SR283 <- cmp GSR1294(a6) (0)
                        GSR277(a7)[A1] <- SR283[P1] SR278
                        GSR277(a7)[A1] <- SR282[P1] SR280
                        br L22

                GSR277 was cond-defined and should be considered as an USE,
                but data-flow solver cannot distingwish that because of
                the cond-def. */
                p1--;
                pos1 = p1;
                continue;
            }
            if (lt->is_def(p1)) {
                is_p1_def = true;
            } else {
                is_p1_def = false;
            }
            break;
        }
    }

    proc = true;
    while (proc) {
        if (p2 == (INT)lastpos) {
            is_p2_def = false;
            break;
        } else {
            if (!occ->is_contain(p2)) {
                p2++;
                pos2 = p2;
                continue;
            }
            if (lt->is_def(p2)) {
                IR * ir = m_ltm->get_ir(p2);
                ASSERT0(ir);
                if (ir->is_select()) {
                    /* CASE: 20020402-3.c:blockvector_for_pc_sect():BB10
                        gsr275(a3) lived in and lived out.
                        first pos:0
                        last pos:12, cond def

                        sr268[A1] :- lw_m sr97(p0)[P1] gsr263(a4)[A1] (0x0)
                        sr266(d10)[D2] :- lw_m sr97(p0)[P1] sr268[A1] (0x8)
                        sr267(d2)[D1] :- lw_m sr97(p0)[P1] sr268[A1] (0xc)
                        ...
                        gsr275(a3)[A1] gsr271(p7)[P1] gsr272(p6)[P1] :- sgtu_m sr270(p1)[P1] ...

                    The spliting candidate is GSR275.
                    Although the operator at postition p2 is a DEF,
                    but it was a conditional DEF! So we regard position p2 as
                    an USE in order to insert a reloading before the cond DEF and
                    add a spilling followed the FIRST position to supply the spill
                    temp memory location.
                    result code can be:
                        FIRST position
                        sw_m gsr275(a3)[A1], gra_spill_temp
                        ...
                        gsr275(a3)[A1] = lw_m gra_spill_temp
                        gsr275(a3)[A1] gsr271(p7)[P1] gsr272(p6)[P1] :- sgtu_m sr270(p1)[P1] ...
                    */
                    is_p2_def = false;
                } else {
                    is_p2_def = true;
                }
            } else {
                is_p2_def = false;
            }
            break;
        }
    }

    //4 plots
    if (is_p1_def && !is_p2_def) {         //def ... use
        is_pos1_spill = true; //need spill
        is_pos2_spill = false; //need reload
        return;
    } else if (is_p1_def && is_p2_def) { //def ... def
        pos1 = pos2 = -1; //do not need spill and reload.
        return;
    } else if (!is_p1_def) {
        if (is_p2_def) {                 //use ... def
            //pos2 do not need reload.
            pos2 = -1;
        } else {                         //use ... use
            is_pos2_spill = false; //need reload at pos2
        }

        //Find the DEF of pos1.
        pos1 = lt->get_backward_def_occ(p1, firstpos);
        if (pos1 != -1) {
            is_pos1_spill = true; //spill at pos1
        } else if ( //LT_is_dedicated(lt) ||
                   LT_is_global(lt)) { //Might be live-in lifetime
            pos1 = p1;
            is_pos1_spill = true; //spill at pos1
        } else {
            ASSERT(0, ("local use without DEF, dead use?"));
        }
        return;
    }
    ASSERT(0, ("Should we here?"));
}


//Return true if ir is the one of operands of 'ir' , and is also the result.
//'prno': can be NULL. And if it is NULL, we only try to
//get the index-info of the same opnd and result.
bool BBRA::isOpndSameWithResult(IR *)
{
    UNREACH();
    return false;
}


void BBRA::renameResult(IR *, UINT old_prno, IR * newpr)
{
    UNUSED(newpr);
    UNUSED(old_prno);
    UNREACH();
}


void BBRA::renameOpnd(IR *, UINT old_prno, IR * newpr)
{
    UNUSED(newpr);
    UNUSED(old_prno);
    UNREACH();
}


//Rename opnds in between 'start' and 'end' occurrencens within lifetime.
//'start': start pos in lifetime, can NOT be the livein pos.
//'end': end pos in lifetime, can NOT be the liveout pos.
void BBRA::renameOpndInRange(LT * lt, IR * newpr, INT start, INT end)
{
    ASSERT0(lt && newpr && newpr->is_pr());
    UNREACH();
    INT firstpos = m_ltm->get_first_pos();
    INT lastpos = m_ltm->get_last_pos();
    if (start == -1) { start = firstpos; }
    if (end == -1) { end = lastpos; }
    ASSERT0(start >= firstpos && start <= lastpos);
    ASSERT0(end >= firstpos && end <= lastpos);
    ASSERT0(start <= end);

    BitSet * occ = LT_occ(lt);
    ASSERT0(occ);
    start = MAX(start, occ->get_first());
    end = MIN(end, occ->get_last());
    if (!occ->is_contain(start)) {
        start = occ->get_next(start);
    }
    if (start == -1) { return; }
    for (INT i = start; i <= end; i = occ->get_next(i)) {
        ASSERT(i >= 0, ("out of boundary"));
        IR * ir = m_ltm->get_ir(i);
        ASSERT0(ir);
        if (lt->is_def(i)) {
            renameResult(ir, LT_prno(lt), newpr);
        } else {
            renameOpnd(ir, LT_prno(lt), newpr);
        }
    }
}


//Generate spilling and reloading code at position 'start' and 'end'
//of life time 'lt' respectively.
//'lt': split candidate, may be local and global lifetimes.
//NOTICE: Neglact 'start' if it equals -1, and similar for 'end'.
void BBRA::splitLTAt(
        INT start,
        INT end,
        bool is_start_spill,
        bool is_end_spill,
        LT * lt)
{
    ASSERT0(lt);
    INT firstpos = m_ltm->get_first_pos();
    INT lastpos = m_ltm->get_last_pos();
    IR * spill_loc = NULL;
    if (start != -1) {
        if (start == firstpos) {
            spill_loc = m_ltm->genSpill(lt, start);
        } else {
            ASSERT0(m_ltm->get_ir(start));
            if (is_start_spill) { //Store to memory
                spill_loc = m_ltm->genSpill(lt, start);
            } else { //Reload from memory
                ASSERT(0, ("Reload at the start position "
                            "of Hole? It will be performance Gap!"));
                //m_ltm->genReload(lt, start);
            }
        }
    }

    if (end != -1) {
        if (end == lastpos) {
            ASSERT0(spill_loc);
            IR * newpr = m_ltm->genReload(lt, end, spill_loc);
            CK_USE(newpr);
            if (LT_is_global(lt)) {
                ASSERT(PR_no(newpr) == LT_prno(lt),
                        ("Should not rename global register, since that "
                         "global information needs update."));
            }
        } else {
            ASSERT0(m_ltm->get_ir(end));
            if (is_end_spill) { //Store to memory
                //I think hereon that operations should be reloading!
                ASSERT(0,
                    ("Store at the end position of Hole? Performance Gap!"));
                //m_ltm->genSpill(lt, end);
            } else { //Reload from memory
                ASSERT0(spill_loc);
                IR * newpr = m_ltm->genReload(lt, end, spill_loc);
                if (PR_no(newpr) != LT_prno(lt)) {
                    //Do renaming.
                    INT forward_def = lt->get_forward_def_occ(end, firstpos);

                    //May be same result as operand.
                    if (forward_def != -1 && forward_def == (end + 1)) {
                        IR * occ = m_ltm->get_ir(end);
                        ASSERT(m_ltm->get_ir(forward_def) == occ,
                                ("o should be same result and operand."));
                        if (!isOpndSameWithResult(occ)) {
                            //Generate new sr again.
                            newpr = m_ru->buildPR(IR_dt(newpr));
                        }

                        //Rename all follows REFs.
                        forward_def = -1;
                    }
                    if (forward_def != -1) {
                        renameOpndInRange(lt, newpr, end, forward_def - 1);
                    } else {
                        renameOpndInRange(lt, newpr, end, -1);
                    }
                }
            }
        }
    }
}


bool BBRA::split(LT * lt)
{
    ASSERT0(!LT_is_global(lt)); //glt already has alllocated.
    bool has_hole;
    LT * cand = computeSplitCand(lt, has_hole, m_tmp_lts, m_tmp_lts2);
    ASSERT0(cand);
    if (has_hole && cand != lt) {
        INT start, end;
        bool find = find_hole(start, end, cand, lt);
        if (find) {
            bool is_start_spill, is_end_spill;
            selectReasonableSplitPos(start, end, is_start_spill,
                                        is_end_spill, cand);
            splitLTAt(start, end, is_start_spill, is_end_spill, cand);
            return true;
        }
    }


    //realloc(lt);
    /*
    bool has_hole = false;
    LifeTime * cand =
        computeBestSpillCand(lt, ig, mgr, true, &has_hole);

    INT hole_startpos, hole_endpos;
    bool split_hole = false;
    if (has_hole && cand != lt) {
        split_hole =
            get_residedin_hole(&hole_startpos, &hole_endpos, cand, lt, mgr);
    }

    if (split_hole) {
        bool is_start_spill, is_end_spill;
        selectReasonableSplitPos(&hole_startpos,
                                    &hole_endpos,
                                    &is_start_spill,
                                    &is_end_spill,
                                    cand, mgr);
        splitLTAt(hole_startpos,
                    hole_endpos,
                    is_start_spill,
                    is_end_spill,
                    cand, mgr);
    } else if (lt == cand) {
        split_one_lt(lt, prio_list,
                    uncolored_list, mgr,
                    ig, spill_location, action);
    } else if (!can_be_spilled(lt, mgr)) {
        split_one_lt(cand, prio_list,
                    uncolored_list, mgr,
                    ig, spill_location, action);
    } else {
        split_two_lts(lt, cand, prio_list,
                    uncolored_list, mgr, ig,
                    spill_location, action);
    }

    show_phase("---Split,before ReAllocate_LifeTime");
    reallocate_lifetime(prio_list, uncolored_list,
                        mgr, ddg, layerddg, ig, cri);
    for (LifeTime * tmplt = uncolored_list.get_head();
         tmplt != NULL; tmplt = uncolored_list.get_next()) {
        if (HAVE_FLAG(m_cur_phase, PHASE_FINIAL_FIXUP_DONE)) {
            //Should not change regfile again.
            action.set_action(tmplt, ACTION_SPLIT);
        } else {
            action.set_action(tmplt, ACTION_BFS_REASSIGN_REGFILE);
        }
    }
    show_phase("---Split finished");
    */
    return true;
}


void BBRA::dump_prio(List<LT*> & prios)
{
    if (g_tfile == NULL) { return; }
    for (LT * l = prios.get_head(); l != NULL; l = prios.get_next()) {
        fprintf(g_tfile, "\nLT%d(pr%d)(prio=%f)",
                LT_uid(l), LT_prno(l), LT_priority(l));
        if (LT_is_global(l)) {
            fprintf(g_tfile, "(global)");
        }
        if (l->has_allocated()) {
            fprintf(g_tfile, "(phy=v%d)", LT_phy(l));
        }
        if (LT_prefer_reg(l) != REG_UNDEF) {
            fprintf(g_tfile, "(prefer=v%d)", LT_prefer_reg(l));
        }
    }
    fflush(g_tfile);
}


bool BBRA::solve(List<LT*> & prios)
{
    ASSERT0(m_tmp_lts && m_tmp_uints);
    for (;prios.get_elem_count() > 0;) {
        LT * lt = prios.remove_head();
        ASSERT(!lt->has_branch(m_ltm),
        ("Branch should be allocated first, we can not split at branch"));
        bool succ = split(lt);
        CK_USE(succ);
        ASSERT0(succ);
        m_ltm->clean();
        m_ltm->build(true, NULL, *m_tmp_cii);
        m_rsc->comp_local_usage(m_ltm, true, m_omit_constrain);

        List<LT*> * unalloc = m_tmp_lts;
        collectUnalloc(*unalloc);
        if (unalloc->get_elem_count() == 0) { return true; }

        m_ltm->get_ig()->erase();
        m_ltm->get_ig()->build();

        prios.clean();
        buildPrioList(*unalloc, prios);
        dump_prio(prios);

        List<UINT> * nis = m_tmp_uints;
        allocPrioList(prios, *nis);
    }
    return false;
}


void BBRA::collectUnalloc(List<LT*> & unalloc)
{
    Vector<LT*> * vec = m_ltm->get_lt_vec();
    unalloc.clean();
    for (INT i = 1; i <= vec->get_last_idx(); i++) {
        LT * lt = vec->get(i);
        if (lt == NULL || lt->has_allocated()) { continue; }
        ASSERT(!LT_is_global(lt), ("glt should be allocated already"));
        unalloc.append_head(lt);
    }
}


bool BBRA::perform(List<LT*> & prios)
{
    ASSERT0(m_ra->m_gltm.map_bb2ltm(m_bb));
    ASSERT0(m_tmp_lts);

    List<LT*> * unalloc = m_tmp_lts;
    collectUnalloc(*unalloc);
    if (unalloc->get_elem_count() == 0) { return true; }

    prios.clean();
    buildPrioList(*unalloc, prios);

    ASSERT0(m_tmp_uints);
    List<UINT> * nis = m_tmp_uints;
    allocPrioList(prios, *nis);
    if (prios.get_elem_count() != 0) {
        //Use m_tmp_lts, m_tmp_lts2, m_tmp_bs
        solve(prios);
    }
    return true;
}
//END BBRA


static int gcount = 0;
static bool gdebug()
{
    return 1;
}


//
//START RA
//
void RA::allocLocal(List<UINT> & nis, bool omit_constrain)
{
    List<LT*> unalloc;
    List<LT*> tmp;
    List<LT*> prios;
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        BBRA l(bb, this);
        l.set_tmp_lts(&unalloc);
        l.set_tmp_lts2(&tmp);
        l.set_tmp_uints(&nis);
        l.set_tmp_cii(&m_cii);
        l.set_omit_constrain(omit_constrain);
        l.perform(prios);
    }
}


//Alloc phy-register for local lt which has specific constrain.
void RA::allocLocalSpec(List<UINT> & nis)
{
    BBList * bbl = m_ru->get_bb_list();
    Vector<IR*> need_to_alloc;
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        IR * lastir = BB_last_ir(bb);
        if (lastir == NULL || (!lastir->is_cond_br() &&
            !lastir->is_uncond_br()    && !lastir->is_multicond_br())) {
            continue;
        }

        m_ii.clean();
        LTMgr * ltm = m_gltm.get_ltm(BB_id(bb));
        need_to_alloc.clean();
        UINT idx = 0;
        bool need_rebuild = false;
        for (IR * k = iterInit(lastir, m_ii); k != NULL; k = iterNext(m_ii)) {
            if (!k->is_pr()) { continue; }

            LT * l = ltm->map_pr2lt(PR_no(k));
            ASSERT0(l);
            if (LT_is_global(l) && !l->has_allocated()) {
                //Can not leave the work to gra.
                IR * newk = insertMoveBefore(lastir, k);
                need_to_alloc.set(idx, newk);
                idx++;
                need_rebuild = true;
                continue;
            }
            if (l->has_allocated()) {
                ASSERT0(LT_usable(l) && LT_usable(l)->is_contain(LT_phy(l)));
                continue;
            }
            need_to_alloc.set(idx, k);
            idx++;
        }

        if (idx == 0) { continue; }
        if (need_rebuild) {
            //TODO: add lt incremental.
            ltm->clean();
            ltm->build(true, NULL, m_cii);
            ltm->get_ig()->erase();
            ltm->get_ig()->build();
        }

        BBRA lra(bb, this);
        lra.set_omit_constrain(false);
        for (UINT i = 0; i < idx; i++) {
            IR * x = need_to_alloc.get(i);
            ASSERT0(x);
            LT * lx = ltm->map_pr2lt(PR_no(x));
            ASSERT0(lx);
            m_rsc.comp_lt_usable(lx, ltm);
            lra.assignRegister(lx, nis);
            ASSERT(lx->has_allocated(), ("not enough phy"));
            ASSERT0(LT_usable(lx)->is_contain(LT_phy(lx)));
            updateLTMaxReg(lx);
        }
    }
}


//Compute priority of glt.
float RA::computePrio(GLT * g)
{
    if (GLT_bbs(g) == NULL) { return 0.0; }
    float prio;

    //TODO: enable it.
    //prio = ((float)g->computeNumOfOcc(m_gltm)) /
    //        (float)GLT_bbs(g)->get_elem_count();
    //prio *= GLT_freq(g);

    //Check to see if glt has branch occ.
    UINT cross_branch = 0;
    DefDBitSetCore * bbs = GLT_bbs(g);
    SEGIter * sc = NULL;
    for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
        LTMgr * ltm = m_gltm.get_ltm(j);
        ASSERT0(ltm); //glt miss local part.
        LT * gl = ltm->map_pr2lt(GLT_prno(g));
        if (gl->has_branch(ltm)) {
            cross_branch++;
        }
    }

    prio = (float)GLT_bbs(g)->get_elem_count();
    BitSet const* usable = GLT_usable(g);
    ASSERT(usable, ("miss usable-regs info"));
    if (m_rsc.get_4() == usable) {
        prio += 1600;
    } else if (m_rsc.get_8() == usable) {
        prio += 800;
    } else if (m_rsc.get_16() == usable) {
        prio += 100;
    }
    if (GLT_rg_sz(g) > 1) {
        prio *= (float)(GLT_rg_sz(g) * 2);
    }
    if (cross_branch > 0) {
        prio *= 1000 * cross_branch;
    }
    return prio;
}


/* Compute priority list and sort life times with descending order of priorities.
We use some heuristics factors to evaluate the
priorities of each of life times:
    1. Life time in critical path will be put in higher priority.
    2. Life time whose symbol register referenced in
        high density will have higher priority.
    3. Life time whose usable registers are fewer, the priority is higher.

    TO BE ESTIMATED:
        Longer life time has higher priority.
*/
void RA::buildPrioList(OUT List<GLT*> & prios)
{
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL || GLT_is_param(g) || g->has_allocated()) {
            //Allocate parameter elsewhere.
            continue;
        }
        GLT_prio(g) = computePrio(g);

        //Search for appropriate position to place.
        GLT * t;
        C<GLT*> * ct;
        for (t = prios.get_head(&ct); t != NULL; t = prios.get_next(&ct)) {
            if (GLT_prio(t) < GLT_prio(g)) {
                break;
            }
        }
        if (t == NULL) {
            prios.append_tail(g);
        } else {
            prios.insert_before(g, ct);
        }
    }
}


void RA::diffLocalNeighbourUsed(GLT * g, List<UINT> & nis, BitSet * unusable)
{
    DefDBitSetCore * bbs = GLT_bbs(g);
    if (bbs == NULL) { return; }

    SEGIter * sc = NULL;
    for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
        LTMgr * ltm = m_gltm.get_ltm(j);
        ASSERT0(ltm); //glt miss local part.

        LT * gl = ltm->map_pr2lt(GLT_prno(g));
        ASSERT0(gl); //glt miss local part.

        IG * ig = ltm->get_ig();
        ASSERT0(ig);

        nis.clean();
        bool on = ig->get_neighbor_list(nis, LT_uid(gl));
        CK_USE(on);
        ASSERT0(on);
        UINT n = nis.get_elem_count();
        for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
            LT * ni = ltm->get_lt(i);
            ASSERT0(ni);
            if (LT_is_global(ni) || !ni->has_allocated()) { continue; }
            if (LT_rg(ni) != NULL) {
                ASSERT0(LT_rg(ni)->get(0) == LT_phy(ni));
                for (UINT i = 0; i < LT_rg(ni)->rnum; i++) {
                    ASSERT0(LT_rg(ni)->get(i) != REG_UNDEF);
                    unusable->bunion(LT_rg(ni)->get(i));
                }
            } else {
                unusable->bunion(LT_phy(ni));
                if (LT_rg_sz(ni) != 1) {
                    ASSERT0(LT_rg_sz(ni) == RG_PAIR_SZ);
                    unusable->bunion(LT_phy(ni) + 1);
                }
            }
        }
    }
}


//Return true if allocation was successful, otherwise return false.
//When register assigned to 'g', it must be deducted from
//the usable_register_set of all its neighbors.
//'unusable': for tmp use.
bool RA::assignRegister(GLT * g, List<UINT> & nis, List<UINT> & nis2)
{
    BitSet const* usable = GLT_usable(g);
    if (usable == NULL) { return false; }
    BitSet * unusable = m_gltm.get_bs_mgr()->create();

    //Avoid allocate the register used by global neighbors.
    nis.clean();
    bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
    CK_USE(on);
    ASSERT0(on);
    UINT n = nis.get_elem_count();
    for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
        GLT * ni = m_gltm.get_glt(i);
        ASSERT0(ni);
        if (!ni->has_allocated()) { continue; }
        if (GLT_rg(ni) != NULL) {
            ASSERT0(GLT_rg(ni)->get(0) == GLT_phy(ni));
            for (UINT i = 0; i < GLT_rg(ni)->rnum; i++) {
                ASSERT0(GLT_rg(ni)->get(i) != REG_UNDEF);
                unusable->bunion(GLT_rg(ni)->get(i));
            }
        } else {
            unusable->bunion(GLT_phy(ni));
            if (GLT_rg_sz(ni) != 1) {
                ASSERT0(GLT_rg_sz(ni) == RG_PAIR_SZ);
                unusable->bunion(GLT_phy(ni) + 1);
            }
        }
    }

    //Avoid allocate the register used by local neighbors.
    diffLocalNeighbourUsed(g, nis2, unusable);

    //Select preference register.
    UINT pref_reg = GLT_prefer_reg(g);
    if (pref_reg != REG_UNDEF && !unusable->is_contain(pref_reg)) {
        if (GLT_rg_sz(g) > 1) {
            ASSERT0(GLT_rg_sz(g) == RG_PAIR_SZ);
            if (!unusable->is_contain(pref_reg + 1)) {
                GLT_phy(g) = (USHORT)pref_reg;
                m_gltm.get_bs_mgr()->free(unusable);
                ASSERT0(usable->is_contain(pref_reg));
                ASSERT0(!is_cross_param(pref_reg, GLT_rg_sz(g)));
                return true;
            }
        } else {
            GLT_phy(g) = (USHORT)pref_reg;
            m_gltm.get_bs_mgr()->free(unusable);
            ASSERT0(usable->is_contain(pref_reg));
            return true;
        }
    }

    //Avoid allocating the registers which
    //are neighbors anticipated.
    n = nis.get_elem_count();
    for (UINT i = nis.get_head(); n > 0; i = nis.get_next(), n--) {
        GLT * ni = m_gltm.get_glt(i);
        ASSERT0(ni);
        if (ni->has_allocated()) { continue; }
        ASSERT0(GLT_usable(ni));

        //Avoid select the reg which ni preferable.
        UINT p = GLT_prefer_reg(ni);
        if (p != REG_UNDEF) {
            unusable->bunion(p);
            if (GLT_rg_sz(ni) != 1) {
                ASSERT0(GLT_rg_sz(ni) == RG_PAIR_SZ);
                unusable->bunion(p + 1);
            }
        }

        /*
        //Only aware of those usable registers more fewer than current.
        //This rule will incur the lt to be allocated some higher phy.
        //e.g: Lt will be assigned v256 or more higher.
        if (ni_usable != NULL &&
            ni_usable->get_elem_count() < usable->get_elem_count()) {
            unusable->bunion(*ni_usable);
        }
        */
    }

    /*
    UINT reg = REG_UNDEF;
    BitSet * anti = m_gltm.get_anti_regs(g, false);
    if (anti != NULL) {
        //Allocate register that 'g' anticipated.
        for (INT r = anti->get_first(); r >= 0; r = anti->get_next(r)) {
            if (usable->is_contain(r)) {
                GLT_phy(g) = r;
                return true;
            }
        }
    }
    */

    bool succ = false;
    INT m = usable->get_last();
    for (INT i = FIRST_PHY_REG; i <= m; i++) {
        if (!unusable->is_contain(i)) {
            if (GLT_rg_sz(g) != 1) {
                ASSERT(GLT_rg_sz(g) == RG_PAIR_SZ, ("to support more size"));
                if (!unusable->is_contain(i + 1) &&
                    !is_cross_param(i, GLT_rg_sz(g))) {
                    GLT_phy(g) = (USHORT)i;
                    succ = true;
                    break;
                }
            } else {
                GLT_phy(g) = (USHORT)i;
                succ = true;
                break;
            }
        }
    }
    m_gltm.get_bs_mgr()->free(unusable);
    ASSERT0(!succ || usable->is_contain(GLT_phy(g)));
    return succ;
}


void RA::allocParameter()
{
    UINT vreg = m_maxreg;
    m_param_reg_start = m_maxreg;
    for (UINT i = 0; i < m_param_num; i++, vreg++) {
        UINT prno = m_gltm.m_params.get(i);
        if (prno == 0) { continue; }
        GLT * g = m_gltm.map_pr2glt(prno);
        ASSERT0(g && !g->has_allocated());
        GLT_phy(g) = (USHORT)vreg;
        ASSERT0(GLT_usable(g) && GLT_usable(g)->is_contain(vreg));
        g->set_local(m_gltm);
    }
    m_maxreg += m_param_num;
}


UINT RA::computeReserveRegister(IRIter & ii, List<IR*> & resolve_list)
{
    UINT max_rescount = 0;
    resolve_list.clean();
    List<IRBB*> * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.get_ltm(BB_id(bb));
        if (ltm == NULL) { continue; }
        for (IR * ir = BB_first_ir(bb);
             ir != NULL; ir = BB_next_ir(bb)) {
            ii.clean();
            FMT fmt = m_rsc.get_fmt(ir);
            ASSERT0(fmt != FUNDEF);
            UINT rescount = 0;

            if (ir->is_stpr() || ir->isCallHasRetVal()) {
                LT * l = ltm->map_pr2lt(ir->get_prno());
                BitSet const* usable = m_rsc.get_usable(fmt, true);
                ASSERT(usable, ("stmt miss usable-regs info"));
                if (LT_rg_sz(l) > 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    if (!usable->is_contain(LT_phy(l) + 1)) {
                        rescount += 2;
                    }
                } else if (!usable->is_contain(LT_phy(l))) {
                    rescount++;
                }
            }

            for (IR * k = iterRhsInit(ir, ii);
                 k != NULL; k = iterRhsNext(ii)) {
                if (!k->is_pr()) { continue; }

                LT * l = ltm->map_pr2lt(PR_no(k));
                ASSERT0(l);

                BitSet const* usable = m_rsc.get_usable(fmt, false);
                ASSERT(usable, ("stmt miss usable-regs info"));
                if (LT_rg_sz(l) > 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    if (!usable->is_contain(LT_phy(l) + 1)) {
                        rescount += 2;
                    }
                } else if (!usable->is_contain(LT_phy(l))) {
                    rescount++;
                }
            }

            ASSERT0(rescount <= 16);
            if (rescount != 0) {
                resolve_list.append_tail(ir);
            }
            max_rescount = MAX(max_rescount, rescount);
        }
    }
    return max_rescount;
}


//Return true if PR need to be spilled.
bool RA::checkIfNeedSpill(UINT prno, FMT fmt, LTMgr const* ltm)
{
    LT * l = ltm->map_pr2lt(prno);
    BitSet const* usable = m_rsc.get_usable(fmt, true);
    ASSERT(usable, ("stmt miss usable-regs info"));

    if (!usable->is_contain(LT_phy(l))) {
        return true;
    }

    if (LT_rg_sz(l) > 1) {
        ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
        if (!usable->is_contain(LT_phy(l) + 1)) {
            return true;
        }
    }
    return false;
}


//Revise resource constrain.
void RA::reviseRSC()
{
    UINT resc = 0;
    List<IR*> resolve_list;
    for (;;) {
        UINT c = computeReserveRegister(m_ii, resolve_list);
        UINT ofst = 0;
        if (c <= resc) {
            break;
        }
        ofst = c - resc;
        resc = c;
        shiftReg(ofst);
    }

    if (resc == 0) { return; }

    BitSet visitbb;
    IR2INT pr2phy;
    for (IR * ir = resolve_list.get_head();
         ir != NULL; ir = resolve_list.get_next()) {
        IRBB * irbb = ir->get_bb();
        LTMgr * ltm = m_gltm.get_ltm(BB_id(irbb));
        visitbb.bunion(BB_id(irbb));
        FMT fmt = m_rsc.get_fmt(ir);
        ASSERT0(fmt != FUNDEF);
        m_ii.clean();

        //Indicate the phy reg num of lhs/rhs which is going to assign.
        INT lhsn = 0;
        INT rhsn = 0;

        if (ir->is_stpr() || ir->isCallHasRetVal()) {
            ASSERT(lhsn < 1, ("multiple def"));
            UINT prno = ir->get_prno();
            if (checkIfNeedSpill(prno, fmt, ltm)) {
                IR * spill_loc = ltm->genSpillSwap(ir, prno, IR_dt(ir), NULL);
                ASSERT0(spill_loc->get_stmt());
                pr2phy.set(spill_loc, lhsn);
                LT * l = ltm->map_pr2lt(prno);
                lhsn += LT_rg_sz(l);
            }
        }

        for (IR * k = iterRhsInit(ir, m_ii);
             k != NULL; k = iterRhsNext(m_ii)) {
            if (!k->is_pr()) { continue; }

            LT * l = ltm->map_pr2lt(PR_no(k));
            ASSERT0(l);

            BitSet const* usable = m_rsc.get_usable(fmt, false);
            ASSERT(usable, ("stmt miss usable-regs info"));

            if (usable->is_contain(LT_phy(l))) {
                if (LT_rg_sz(l) > 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    if (usable->is_contain(LT_phy(l) + 1)) {
                        continue;
                    }
                } else {
                    continue;
                }
            }

            IR * spill_loc = ltm->genReloadSwap(k, ir);
            ASSERT0(spill_loc->get_stmt());
            pr2phy.set(spill_loc, rhsn);
            ASSERT0(LT_rg_sz(l) == 1 || LT_rg_sz(l) == 2);
            rhsn += LT_rg_sz(l);
        }
        ASSERT0(lhsn <= (INT)resc && rhsn <= (INT)resc);
    }

    //Rebuild lt for new PR which generated by above step.
    for (INT bbid = visitbb.get_first();
         bbid >= 0; bbid = visitbb.get_next(bbid)) {
        LTMgr * ltm = m_gltm.get_ltm(bbid);
        ASSERT0(ltm);
        ltm->clean();
        ltm->build(false, NULL, m_cii);
        #ifdef _DEBUG_
        //The interf graph is useless after all resolved. Only for verify used.
        ltm->get_ig()->erase();
        ltm->get_ig()->build();
        #endif
    }

    //Bind the new PR with the specified phy register.
    TMapIter<IR*, INT> iter2;
    INT phy;
    for (IR * ir = pr2phy.get_first(iter2, &phy);
         ir != NULL; ir = pr2phy.get_next(iter2, &phy)) {
        IR * stmt = ir->get_stmt();
        ASSERT0(stmt && stmt->get_bb());
        LTMgr * ltm = m_gltm.get_ltm(BB_id(stmt->get_bb()));
        ASSERT0(ltm);
        LT * l = ltm->map_pr2lt(PR_no(ir));
        ASSERT0(l && LT_prno(l) == PR_no(ir) && !l->has_allocated());
        LT_phy(l) = (USHORT)phy;
    }
}


void RA::shiftReg(UINT ofst)
{
    ASSERT0(ofst < 60000);
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL) { continue; }
            ASSERT0(l->has_allocated());
            LT_phy(l) += (USHORT)ofst;
        }
    }

    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        ASSERT0(g->has_allocated());
        GLT_phy(g) += (USHORT)ofst;
    }

    m_maxreg += ofst;
}


void RA::rotateReg()
{
    ASSERT0(m_param_reg_start == 0);
    if (m_param_num == 0 || m_gltm.m_params.get_last_idx() == -1) {
        return;
    }
    BBList * bbl = m_ru->get_bb_list();
    UINT regn = m_maxreg + 1;
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL) { continue; }
            ASSERT0(l->has_allocated());

            if (LT_phy(l) < m_param_num) {
                LT_phy(l) = (USHORT)(regn - m_param_num) + LT_phy(l);
            } else {
                LT_phy(l) = LT_phy(l) - (USHORT)m_param_num;
            }
        }
    }
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        ASSERT0(g->has_allocated());

        if (GLT_phy(g) < m_param_num) {
            GLT_phy(g) = (USHORT)(regn - m_param_num) + GLT_phy(g);
        } else {
            GLT_phy(g) = GLT_phy(g) - (USHORT)m_param_num;
        }
    }
}


void RA::reviseParam()
{
    if (m_param_num == 0 ||
        m_param_reg_start + m_param_num - 1 == m_maxreg ||
        m_gltm.m_params.get_last_idx() == -1) { //not any param in use.
        //Parameter already in place.
        return;
    }
    UINT vreg = m_maxreg - m_param_num + 1;
    UINT lastp = m_param_reg_start + m_param_num - 1;
    m_param_reg_start = vreg;
    if (vreg <= lastp) {
        vreg = lastp + 1;
    }
    List<LT*> nis;
    BitSet visit;

    #ifdef _DEBUG_
    BitSet params;
    for (UINT i = 0; i < m_param_num; i++) {
        UINT prno = m_gltm.m_params.get(i);
        params.bunion(prno);
    }
    #endif

    //Swap phy for each lived bb.
    UINT i;
    for (i = 0; vreg <= m_maxreg; i++, vreg++) {
        UINT prno = m_gltm.m_params.get(i);
        if (prno == 0) { continue; }
        GLT * g = m_gltm.map_pr2glt(prno);
        ASSERT0(g && g->has_allocated());
        DefDBitSetCore * bbs = GLT_bbs(g);
        if (bbs == NULL) { continue; }
        SEGIter * sc = NULL;

        UINT gphy = GLT_phy(g);
        for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
            LTMgr * ltm = m_gltm.get_ltm(j);
            if (ltm == NULL) { continue; }
            LT * gl = ltm->map_pr2lt(GLT_prno(g));
            ASSERT0(gl); //glt miss local part.

            nis.clean();
            ltm->get_ig()->get_neighbor(nis, gl);
            for (LT * ni = nis.get_head(); ni != NULL; ni = nis.get_next()) {
                if (LT_phy(ni) != vreg) { continue; }
                LT_phy(ni) = (USHORT)gphy;
                #ifdef _DEBUG_
                //This neighbour should not be parameter.
                ASSERT0(!params.is_contain(LT_prno(ni)));
                #endif
                if (LT_is_global(ni) && !visit.is_contain(LT_prno(ni))) {
                    GLT * niglt = ni->set_global(m_gltm);
                    niglt->set_local(m_gltm);
                    visit.bunion(LT_prno(ni));
                }
            }
        }
        GLT_phy(g) = (USHORT)(m_param_reg_start + i);
        g->set_local(m_gltm);
    }

    //Settle the remainder parameters in.
    for (; i < m_param_num; i++) {
        UINT prno = m_gltm.m_params.get(i);
        if (prno == 0) { continue; }
        GLT * g = m_gltm.map_pr2glt(prno);
        ASSERT0(g && g->has_allocated());
        GLT_phy(g) = (USHORT)(m_param_reg_start + i);
        g->set_local(m_gltm);
    }
}


//'prio' list should be sorted in descending order.
//'nis': for tmp used.
void RA::allocPrioList(OUT List<GLT*> & prios, OUT List<GLT*> & unalloc,
                         List<UINT> & nis, List<UINT> & nis2)
{
    C<GLT*> * ct, * next_ct;
    for (prios.get_head(&ct), next_ct = ct; ct != NULL; ct = next_ct) {
        GLT * g = ct->val();
        prios.get_next(&next_ct);
        ASSERT0(!g->has_allocated());
        if (!assignRegister(g, nis, nis2)) {
            //The elem in uncolored list are already sorted in
            //descending priority order.
            unalloc.append_tail(g);
        } else {
            ASSERT0(g->has_allocated());
            updateGltMaxReg(g);
        }
        prios.remove(ct);
    }
}


//Split glt into several section.
//Return the spill location.
IR * RA::split(GLT * g)
{
    ASSERT0(!GLT_is_param(g));
    DefDBitSetCore * bbs = GLT_bbs(g);
    ASSERT(bbs, ("should not select an empty candidate to split"));
    SEGIter * sc = NULL;
    IR * spill_loc = NULL;
    for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
        LTMgr * ltm = m_gltm.get_ltm(j);
        if (ltm == NULL) { continue; }
        LT * gl = ltm->map_pr2lt(GLT_prno(g));
        ASSERT0(gl); //glt miss local part.
        if (LT_occ(gl) == NULL) { continue; }
        for (INT i = LT_occ(gl)->get_first();
             i >= 0; i = LT_occ(gl)->get_next(i)) {
            IR * occ = ltm->get_ir(i);
            ASSERT(occ, ("illegal occ info."));
            if (gl->is_def(i)) {
                if (occ->is_stpr()) {
                    ASSERT0(STPR_no(occ) == LT_prno(gl));
                    spill_loc = ltm->genSpill(STPR_no(occ),
                                               IR_dt(occ), occ, spill_loc);
                } else {
                    IR * orgpr = occ->getResultPR(LT_prno(gl));
                    ASSERT0(orgpr && orgpr->is_pr() &&
                             PR_no(orgpr) == LT_prno(gl));
                    spill_loc = ltm->genSpill(PR_no(orgpr),
                                               IR_dt(orgpr), occ, spill_loc);
                }
            } else {
                IR * tgtpr = occ->getOpndPR(LT_prno(gl));
                ASSERT0(tgtpr && tgtpr->is_pr() &&
                         PR_no(tgtpr) == LT_prno(gl));
                if (spill_loc == NULL) {
                    spill_loc = m_ru->dupIR(tgtpr);
                }
                ltm->genReload(tgtpr, occ, spill_loc);
            }
        }
    }
    return spill_loc;
}


//'nis': for tmp used.
void RA::solveConflict(OUT List<GLT*> & unalloc, List<UINT> & nis)
{
    for (GLT * g = unalloc.get_head(); g != NULL; g = unalloc.get_next()) {
        IR * spill_loc = split(g);
        ASSERT0(spill_loc);
        GLT * slglt = m_gltm.buildGltLike(spill_loc, g);
        ASSERT0(slglt);
        GLT_usable(slglt) = m_rsc.get_16();

        nis.clean();
        bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
        CK_USE(on);
        ASSERT0(on);
        m_ig.add_glt(slglt);
        m_ig.set_interf_with(GLT_id(slglt), nis);
        m_ig.remove_glt(g);
        m_gltm.localize(g);
        slglt->set_local_usable(m_gltm);
    }
}


//If current global lt has allocated phy register, then update
//the information to local part to make global and local
//are coherent.
void RA::updateLocal()
{
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        g->set_local(m_gltm);
    }
}


//Verify all glt has been assigned phy.
//Verify local part is consistent with glt.
bool RA::verify_glt(bool check_alloc)
{
    UNUSED(check_alloc);
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        ASSERT0(!check_alloc || g->has_allocated());

        DefDBitSetCore * bbs = GLT_bbs(g);
        if (bbs == NULL) { continue; }

        SEGIter * sc = NULL;
        for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
            LTMgr * ltm = m_gltm.get_ltm(j);

            //If local part not exist, remove the bit out of lived-bbs.
            ASSERT(ltm, ("miss local part"));
            LT * lt = ltm->map_pr2lt(GLT_prno(g));
            CK_USE(lt);

            ASSERT(lt, ("miss local part"));
            ASSERT0(LT_is_global(lt));
            ASSERT0(LT_phy(lt) == GLT_phy(g));
            ASSERT0(LT_rg_sz(lt) == GLT_rg_sz(g));
        }
    }
    return true;
}


//Return true if l intefered with parameters.
bool RA::overlapParam(LT const* l) const
{
    ASSERT0(l->has_allocated());
    return LT_phy(l) >= m_param_reg_start &&
           LT_phy(l) < m_param_reg_start + m_param_num;
}


//Try to compute the maximum number of registers that
//satisfied register group constrains when range starts at 'rangestart'.
//'occupied': phy that has assigned to neighbours of ltg member.
//'assigend': phy that has assigned to ltg member.
UINT RA::computeSatisfiedNumRegister(
        UINT rangestart,
        LTG const* ltg,
        UINT rgsz,
        BitSet const& occupied,
        BitSet const& assigned,
        BitSet const& liveout_phy)
{
    UNUSED(liveout_phy);

    ASSERT0(rangestart != REG_UNDEF);
    UINT nsat = 0;
    if (is_cross_param(rangestart, rgsz)) {
        return 0;
    }

    UINT start = rangestart;
    UINT end = rangestart + rgsz - 1;
    UINT ofst;
    for (INT i = 0; i <= ltg->get_last_idx(); i++, rangestart += ofst) {
        LT * l = ltg->get(i);
        ASSERT0(l);
        ofst = LT_rg_sz(l);
        if (l->has_allocated()) {
            if (LT_phy(l) == rangestart) {
                nsat += LT_rg_sz(l);
                continue;
            }
            if (LT_phy(l) >= start && LT_phy(l) <= end) {
                return 0;
            }
            continue;
        }

        if (occupied.is_contain(rangestart) ||
            assigned.is_contain(rangestart)) {
            //'l' can not obtained the expect one.
            //The tryrange must not contain the
            //phy which has assigned to ltg member.
            return 0;
        }

        if (LT_rg_sz(l) > 1) {
            ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
            if (occupied.is_contain(rangestart + 1) ||
                assigned.is_contain(rangestart + 1)) {
                //l can not obtained the expect one.
                return 0;
            }
        }

        /*
        if (is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy)) {
            //The generated move will override the liveout phy.
            return 0;
        }
        */

        ASSERT0(!is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy));
        nsat += LT_rg_sz(l);
    }
    return nsat;
}


//Compute the number of registers that satisfied register
//group constrains when range starts at 'rangestart'.
//NOTE: This function must find a legal range even if
//inserting move.
UINT RA::computeNumRegister(
        UINT rangestart,
        UINT rangeend,
        LTG const* ltg,
        BitSet const& occupied,
        BitSet const& assigned,
        BitSet const& liveout_phy)
{
    UNUSED(liveout_phy);

    ASSERT0(rangestart != REG_UNDEF);
    UINT start = rangestart;
    UINT nsat = 0;
    for (INT i = 0; i <= ltg->get_last_idx(); i++) {
        LT * l = ltg->get(i);
        ASSERT0(l);
        if (occupied.is_contain(rangestart)) {
            //l can not obtained the expect one.
            return 0;
        }

        if (LT_rg_sz(l) > 1) {
            ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
            if (occupied.is_contain(rangestart + 1)) {
                //l can not obtained the expect one.
                return 0;
            }
        }

        if (l->has_allocated() && LT_phy(l) != rangestart) {
            if (LT_phy(l) >= start && LT_phy(l) <= rangeend) {
                //Will incur generating overlap-copy.
                return 0;
            }
            if (assigned.is_contain(rangestart)) {
                //The tryrange can not contain the
                //phy which has assigned to ltg member.
                return 0;
            }
            if (LT_rg_sz(l) > 1) {
                ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                if (assigned.is_contain(rangestart)) {
                    return 0;
                }
            }
            ASSERT0(!is_cross_liveout_phy(rangestart,
                     LT_rg_sz(l), liveout_phy));
        }

        /*
        if (is_cross_liveout_phy(rangestart, LT_rg_sz(l), liveout_phy)) {
            //The generated move will override the liveout phy.
            return 0;
        }
        */

        nsat += LT_rg_sz(l);
        rangestart += LT_rg_sz(l);
    }
    return nsat;
}


//Return true if the range of register overlapped with live out phy.
//e.g: liveout registers are r0,r1,r10, reigster range is r7~r11,
//that is overlap.
bool RA::is_cross_liveout_phy(UINT reg_start, UINT rgsz, BitSet const& liveout_phy)
{
    for (UINT i = reg_start; i < rgsz; i++) {
        if (liveout_phy.is_contain(i)) {
            return true;
        }
    }
    return false;
}


//Try to find a properly range from those lts which has assigned register.
INT RA::tryReuseAppeared(
        LTG const* ltg,
        BitSet const& occupied,
        BitSet const& assigned,
        BitSet const& liveout_phy)
{
    UINT best_nsat = 0; //The most number of satisfied.
    INT best_range_start = -1;
    INT phyofst = 0;
    UINT rgsz = ltg->get_rg_sz();
    INT frac_start = ((INT)FIRST_PHY_REG) - 1;
    for (INT i = 0; i <= ltg->get_last_idx(); i++) {
        LT const* l = ltg->get(i);
        ASSERT0(l);
        INT rangestart = -1;
        if (l->has_allocated()) {
            rangestart = ((INT)LT_phy(l)) - phyofst;
        }

        if (rangestart < 0) {
            //Try range start from the fraction bewteen the phy
            //which has been used.
            for (INT i = frac_start + 1; i < 1023; i++) {
                if (!occupied.is_contain(i) && !assigned.is_contain(i)) {
                    frac_start = i;
                    break;
                }
            }
            rangestart = frac_start;
        }

        //UINT rangend = rangestart + ltg->get_last_idx();
        UINT nsat = computeSatisfiedNumRegister(rangestart, ltg, rgsz,
                                        occupied, assigned, liveout_phy);
        UINT old = best_nsat;
        best_nsat = MAX(best_nsat, nsat);
        if (best_nsat != old) {
            best_range_start = rangestart;
        }

        if (best_nsat == rgsz) { break; }
        phyofst += (INT)LT_rg_sz(l);
    }

    //best range is permited to cross liveout phy only if there is
    //no move inserted to rememdy the phy.
    ASSERT0(!is_cross_param(best_range_start, rgsz));

    if ((float)best_nsat >= ((float)rgsz * 0.6) && (best_nsat < rgsz)) {
        //int a = 0;
    }
    if ((float)best_nsat < ((float)rgsz * 0.6)) {
    //if (best_nsat < rgsz) {
        return -1;
    }
    return best_range_start;
}


INT RA::tryExtend(
        LTG const* ltg,
        BitSet const& occupied,
        BitSet const& liveout_phy,
        BitSet const& assigned)
{
    //There is not any properly range can be allocated.
    UINT tryrangestart = m_param_reg_start + m_param_num;
    UINT count = 0;
    UINT rgsz = ltg->get_rg_sz();
    while (count < (65535 - rgsz)) {
        if (is_cross_param(tryrangestart, rgsz)) {
            continue;
        }
        UINT tryend = tryrangestart + rgsz - 1;
        INT nsat = (INT)computeNumRegister(tryrangestart, tryend, ltg,
                                  occupied, assigned, liveout_phy);
        if ((UINT)nsat == rgsz) {
            return tryrangestart;
        }

        count++;
        tryrangestart++;
    }
    ASSERT(0, ("invoke-range not allocable!"));
    return -1;
}


//Insert move before ir, and replace origial kid 'src' in 'stmt' with new kid.
//Return the new kid.
//'stmt': stmt of source operand.
//'src': source operand of move, after the replacement, this
//operand will be freed.
IR * RA::insertMoveBefore(IR * stmt, IR * src)
{
    ASSERT0(stmt->is_kids(src));
    IR * newkid = m_ru->buildPR(IR_dt(src));
    IR * mv = m_ru->buildStorePR(PR_no(newkid), IR_dt(newkid),
                                   m_ru->dupIR(src));
    m_rsc.comp_ir_fmt(mv);

    IRBB * irbb = stmt->get_bb();
    ASSERT0(irbb);
    BB_irlist(irbb).insert_before(mv, stmt);
    stmt->replaceKid(src, newkid, true);
    m_ru->freeIRTree(src);
    return newkid;
}


//Check if the phy of ltg member is continuous, and
//insert move if necessary.
//Assign phy if there are members are not assigned.
//'visited': for tmp use
//'nis': for tmp use.
void RA::remedyLTG(
        LTG * ltg,
        IR * ir,
        LTMgr * ltm,
        DefSBitSet & nis,
        BitSet & visited,
        UINT rangestart)
{
    IR * param = CALL_param_list(ir);
    ASSERT0(param);
    while (param != NULL && !param->is_pr()) { param = IR_next(param); }
    ASSERT0(param && cnt_list(param) == ((UINT)ltg->get_last_idx()) + 1);

    TMap<UINT, UINT> prno2phy;
    bool insert = false;
    IR * next = NULL;
    visited.clean();
    UINT org_rangestart = rangestart;

    for (INT i = 0; i <= ltg->get_last_idx(); i++, param = next) {
        next = IR_next(param);
        LT * l = ltg->get(i);
        ASSERT0(l);
        if (l->has_allocated()) {
            visited.bunion(i);
            if (LT_phy(l) != rangestart) {
                IR * x = insertMoveBefore(ir, param);
                LT_ltg(l) = NULL; //l is not ltg member any more.

                //x will be local lt.
                prno2phy.set(PR_no(x), rangestart);
                insert = true;
            }
            ASSERT0(get_maxreg() >= LT_phy(l));
        }
        rangestart += LT_rg_sz(l);
    }
    ASSERT0(param == NULL);

    rangestart = org_rangestart;
    param = CALL_param_list(ir);
    while (param != NULL && !param->is_pr()) { param = IR_next(param); }
    ASSERT0(param && cnt_list(param) == ((UINT)ltg->get_last_idx()) + 1);

    for (INT i = 0; i <= ltg->get_last_idx(); i++, param = next) {
        next = IR_next(param);
        LT * l = ltg->get(i);
        ASSERT0(l && LT_usable(l));
        if (visited.is_contain(i)) {
            rangestart += LT_rg_sz(l);
            continue;
        }

        //If l has allocated, it may be the same pr with previous parameter,
        //which case incurred by copy propagtion.
        bool need_move = false;
        if (l->has_allocated()) {
            #ifdef _DEBUG_
            bool find = false;
            for (INT w = 0; w <= ltg->get_last_idx(); w++) {
                LT * wl = ltg->get(w);
                if (LT_prno(wl) == LT_prno(l)) {
                    find = true;
                    break;
                }
            }
            ASSERT0(find); //Illegal case.
            #endif
            need_move = true;
        }

        if (need_move ||
            (LT_is_global(l) && m_ig.is_interf_with_neighbour(
                                get_glt(LT_prno(l)), nis, rangestart))) {
            /* Only insert a move, leave l to local/global allocator.
               call pr1
            =>
               x = pr1
               call x
            */
            IR * x = insertMoveBefore(ir, param);
            LT_ltg(l) = NULL; //l is not ltg member any more.

            //x will be local lt.
            prno2phy.set(PR_no(x), rangestart);
            insert = true;
        } else if (LT_usable(l)->is_contain(rangestart)) {
            LT_phy(l) = (USHORT)rangestart;
            if (LT_is_global(l)) {
                GLT * g = l->set_global(m_gltm);
                g->set_local(m_gltm);
            }
            updateLTMaxReg(l);
        } else {
            /* Only insert a move, leave l to local/global allocator.
               call pr1
            =>
               x = pr1
               call x
            */
            IR * x = insertMoveBefore(ir, param);
            LT_ltg(l) = NULL; //l is not ltg member any more.

            //x is local lt.
            prno2phy.set(PR_no(x), rangestart);
            insert = true;
        }
        rangestart += LT_rg_sz(l);
    }
    ASSERT0(param == NULL);

    if (insert) {
        //Assigned phy to the swapping vreg.
        ltm->clean();

        //The local part of glt must be rebuild even if it is
        //still not allocated.
        ltm->build(true, NULL, m_cii);
        ltm->get_ig()->erase();
        ltm->get_ig()->build();
        TMapIter<UINT, UINT> iter2;
        UINT phy;
        for (UINT prno = prno2phy.get_first(iter2, &phy);
             prno != 0; prno = prno2phy.get_next(iter2, &phy)) {
            LT * l = ltm->map_pr2lt(prno);
            ASSERT0(l);
            LT_phy(l) = (USHORT)phy;
            updateLTMaxReg(l);
            m_rsc.comp_lt_usable(l, ltm);
            ASSERT0(LT_usable(l) && LT_usable(l)->is_contain(phy));
        }
    }
}


void RA::dump_ltg()
{
    if (g_tfile == NULL) { return; }
    TMapIter<UINT, LTG*> iter;
    LTG * ltg;
    fprintf(g_tfile, "\n=== DUMP LTG === %s ===\n", m_ru->get_ru_name());
    for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
         id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
        IR * ir = m_ru->get_ir(id);
        ASSERT0(ir && is_range_call(ir) && ltg);
        IRBB * bb = ir->get_bb();
        dump_ir(ir, m_tm);
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        ASSERT0(ltm);
        ltm->dump();
    }
    fflush(g_tfile);
}


void RA::assignLTG(LTG * ltg, IR * ir)
{
    ASSERT0(ltg && ltg->ty == LTG_RANGE_PARAM && ir->is_calls_stmt());
    ASSERT0(ir->get_bb());
    IRBB * bb = ir->get_bb();
    //ASSERT0(BB_last_ir(bb) == ir);
    LTMgr * ltm = m_gltm.map_bb2ltm(bb);
    ASSERT0(ltm);

    BitSet assigned; //the phys that ltg has assigned.
    bool all_assigned = true;
    DefSBitSet nis(m_gltm.m_sbs_mgr.get_seg_mgr());
    DefSBitSet gnis(m_gltm.m_sbs_mgr.get_seg_mgr());
    IG * ig = ltm->get_ig();
    ASSERT0(ig);
    BitSet liveout_phy;

    for (INT i = 0; i <= ltg->get_last_idx(); i++) {
        LT * l = ltg->get(i);
        ASSERT0(l);
        ig->get_neighbor_set(nis, LT_uid(l));

        /*
        if (LT_is_global(l)) {
            GLT * g = m_gltm.map_pr2glt(LT_prno(l));
            ASSERT0(g);
            m_ig.get_neighbor_set(gnis, GLT_id(g));
        }
        */

        if (l->has_allocated()) {
            ASSERT0(LT_range(l));
            if (ltm->is_liveout(l)) {
                liveout_phy.bunion(LT_phy(l));
                if (LT_rg_sz(l) > 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    liveout_phy.bunion(LT_phy(l) + 1);
                }
            }

            assigned.bunion(LT_phy(l));

            if (LT_rg_sz(l) > 1) {
                ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                assigned.bunion(LT_phy(l) + 1);
            }
        } else {
            all_assigned = false;
        }
    }

    if (all_assigned) {
        //Check if continuous.
        INT rangestart = -1;
        bool is_cont = true;
        for (INT i = 0; i <= ltg->get_last_idx(); i++) {
            LT * l = ltg->get(i);
            ASSERT0(l && l->has_allocated());
            if (i != 0 && LT_phy(l) != rangestart) {
                //The second condition handle followed case,
                //two lt assigned same phy:
                 //    GLT( 13):  [pr16](v1), rg<pr16(lt13)(v1) pr19(lt16)(v1) >
                is_cont = false;
                break;
            }
            ASSERT0(LT_rg_sz(l) >= 1);
            rangestart = LT_phy(l) + LT_rg_sz(l);
        }
        if (is_cont) {
            return;
        }
    }

    //Compute unusable set.
    BitSet occupied; //occupied vreg by other lt which is not member in group.
    SEGIter * cur = NULL;

    //Compute the phy occupied by local neighbours.
    for (INT ltid = nis.get_first(&cur);
         ltid >= 0; ltid = nis.get_next(ltid, &cur)) {
        LT * l = ltm->get_lt(ltid);
        ASSERT0(l);
        if (!l->has_allocated()) { continue; }
        occupied.bunion(LT_phy(l));
        if (LT_rg_sz(l) != 1) {
            ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
            occupied.bunion(LT_phy(l) + 1);
        }
    }

    /*
    for (INT gltid = gnis.get_first(&cur);
         gltid >= 0; gltid = gnis.get_next(gltid, &cur)) {
        GLT * g = m_gltm.get_glt(gltid);
        ASSERT0(g);
        if (!g->has_allocated()) { continue; }

        occupied.bunion(GLT_phy(g));
        if (GLT_rg_sz(g) != 1) {
            ASSERT0(GLT_rg_sz(g) == RG_PAIR_SZ);
            occupied.bunion(GLT_phy(g) + 1);
        }

    }
    */

    //Compute the phy occupied by global neighbours.
    for (INT i = 0; i <= ltg->get_last_idx(); i++) {
        LT * l = ltg->get(i);
        ASSERT0(l);
        if (!LT_is_global(l)) { continue; }

        GLT * g = m_gltm.map_pr2glt(LT_prno(l));
        ASSERT0(g);
        UINT prno = GLT_prno(g);
        SEGIter * sc = NULL;
        DefDBitSetCore * bbs = GLT_bbs(g);
        if (bbs == NULL) { continue; }

        for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
            LTMgr * nltm = m_gltm.get_ltm(j);
            if (nltm == NULL) { continue; }

            LT * gl = nltm->map_pr2lt(prno);
            ASSERT0(gl); //global lt miss local part.
            IG * nig = nltm->get_ig();
            ASSERT0(nig);
            nis.clean();
            nig->get_neighbor_set(nis, LT_uid(gl));

            //Compute the phy occupied by local part.
            SEGIter * cur2 = NULL;
            for (INT ltid = nis.get_first(&cur2);
                 ltid >= 0; ltid = nis.get_next(ltid, &cur2)) {
                LT * l = nltm->get_lt(ltid);
                ASSERT0(l);
                if (!l->has_allocated()) { continue; }

                occupied.bunion(LT_phy(l));
                if (LT_rg_sz(l) != 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    occupied.bunion(LT_phy(l) + 1);
                }
            }
        }
    }
    occupied.diff(assigned);

    INT best_range_start = tryReuseAppeared(ltg, occupied,
                                              assigned, liveout_phy);
    if (best_range_start == -1) {
        //Find a properly range.
        best_range_start = tryExtend(ltg, occupied, liveout_phy, assigned);
    }
    remedyLTG(ltg, ir, ltm, nis, occupied, best_range_start);
}


//Verify lt's phy satisfied the constrains of occ.
//Note
//  * This function should be invoked after all lts has allocated.
//  * This function does not check LT's usable set, only check instructions.
bool RA::verify_rsc()
{
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }

        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL) { continue; }
            ASSERT0(l->has_allocated());
            if (LT_occ(l) == NULL) { continue; }

            //Do more check for each occ of lt.
            for (INT j = LT_occ(l)->get_first();
                 j >= 0; j = LT_occ(l)->get_next(j)) {
                IR * occ = ltm->get_ir(j);
                ASSERT(occ, ("occ and pos info are not match."));
                FMT fmt = m_rsc.m_ir2fmt.get(IR_id(occ));
                ASSERT0(fmt != FUNDEF);
                BitSet * usable = m_rsc.get_usable(fmt, l->is_def(j));
                CK_USE(usable);

                ASSERT(usable, ("stmt miss usable-regs info"));
                ASSERT(usable->is_contain(LT_phy(l)), ("phy is not legal"));
            }
        }
    }
    return true;
}


//Verify if phy is usable to each lt.
//Verify the local part usable-set must be consistent with global.
bool RA::verify_usable()
{
    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL) { continue; }
            if (LT_occ(l) != NULL) {
                ASSERT0(LT_usable(l));
                if (l->has_allocated()) {
                    ASSERT(LT_usable(l)->is_contain(LT_phy(l)),
                            ("phy is unusable to lt"));
                }
                if (LT_is_global(l)) {
                    GLT * g = m_gltm.map_pr2glt(LT_prno(l));
                    CK_USE(g);
                    ASSERT0(g);
                    ASSERT0(GLT_usable(g) == LT_usable(l));
                }
            }
        }
    }
    return true;
}


//Verify each pr correspond to unique phy.
//Verify max reg assigned.
//Verify usable regs.
bool RA::verify_reg(bool check_usable, bool check_alloc)
{
    Prno2Vreg prno2v(getNearestPowerOf2((UINT)57));
    UINT maxreg = 0;
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }
        if (!check_alloc && !g->has_allocated())  { continue; }
        ASSERT0(g->has_allocated());

        maxreg = MAX(maxreg, (UINT)GLT_phy(g));
        if (GLT_rg_sz(g) > 1) {
            ASSERT0(GLT_rg_sz(g) == RG_PAIR_SZ);
            maxreg = MAX(maxreg, (UINT)GLT_phy(g) + 1);
        }
    }

    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL) { continue; }
            if (!check_alloc && !l->has_allocated())  { continue; }
            ASSERT0(l->has_allocated());

            if (LT_occ(l) != NULL && check_usable) {
                ASSERT0(LT_usable(l));
                ASSERT0(LT_usable(l)->is_contain(LT_phy(l)));
            }

            maxreg = MAX(maxreg, (UINT)LT_phy(l));
            if (LT_rg_sz(l) > 1) {
                ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                maxreg = MAX(maxreg, (UINT)LT_phy(l) + 1);
            }
            bool find;
            UINT v = prno2v.get(LT_prno(l), &find);
            UNUSED(v);
            if (find) {
                //each prno is corresponding to a unqiue vreg.
                ASSERT0(v == LT_phy(l));
            } else {
                prno2v.set(LT_prno(l), LT_phy(l));
            }
        }
    }

    //Used phy may less than maxreg.
    ASSERT0(maxreg <= get_maxreg());
    return true;
}


void RA::allocGroup()
{
    TMapIter<UINT, LTG*> iter;
    LTG * ltg;
    for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
         id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
        IR * ir = m_ru->get_ir(id);
        ASSERT0(ir && is_range_call(ir) && ltg);
        assignLTG(ltg, ir);
    }
}


//Build local BB interference graph.
void RA::buildLocalIG()
{
    for (INT i = 0; i <= m_gltm.m_bb2ltmgr.get_last_idx(); i++) {
        LTMgr * ltm = m_gltm.m_bb2ltmgr.get(i);
        if (ltm == NULL) { continue; }
        ltm->get_ig()->build();
    }
}


//Verify live point for global lt.
//Verify occurrence for local lt.
//Verify each pr should correspond to individual lt.
bool RA::verify_lt_occ()
{
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL) { continue; }

        DefDBitSetCore * bbs = GLT_bbs(g);
        if (bbs == NULL) { continue; }

        SEGIter * sc = NULL;
        UINT prno = GLT_prno(g);
        UNUSED(prno);
        for (INT j = bbs->get_first(&sc); j >= 0; j = bbs->get_next(j, &sc)) {
            DefSBitSetCore * livein = m_prdf.get_livein(j);
            DefSBitSetCore * liveout = m_prdf.get_liveout(j);
            CK_USE(livein);
            CK_USE(liveout);
            ASSERT0(livein->is_contain(prno) || liveout->is_contain(prno));
        }
    }

    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.get_ltm(BB_id(bb));
        if (ltm == NULL) { continue; }
        Vector<LT*> * ltvec = ltm->get_lt_vec();
        for (INT i = 1; i <= ltvec->get_last_idx(); i++) {
            LT * l = ltvec->get(i);
            if (l == NULL) { continue; }

            BitSet * occ = LT_occ(l);
            if (occ == NULL) { continue; }

            UINT prno = LT_prno(l);
            for (INT j = occ->get_first(); j >= 0; j = occ->get_next(j)) {
                IR * ir = ltm->get_ir(j);
                ASSERT0(ir);
                if (l->is_def(j)) {
                    IR * pr = ir->getResultPR(prno);
                    CK_USE(pr);
                } else {
                    IR * pr = ir->getOpndPR(prno);
                    CK_USE(pr);
                }
            }
        }
    }

    ConstIRIter ii;
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.get_ltm(BB_id(bb));
        if (ltm == NULL) { continue; }

        for (IR const* ir = BB_first_ir(bb);
             ir != NULL; ir = BB_next_ir(bb)) {
            ii.clean();
            for (IR const* k = iterInitC(ir, ii);
                 k != NULL; k = iterNextC(ii)) {
                if (!k->is_pr() || !k->is_stpr()) { continue; }

                LT * l = ltm->map_pr2lt(k->get_prno());
                CK_USE(l);
                ASSERT0(LT_prno(l) == k->get_prno());
            }
        }
    }
    return true;
}


//Verify global and local lt has been assigned conflict phy.
bool RA::verify_interf()
{
    List<UINT> nis;
    Vector<GLT*> * gltv = m_gltm.get_gltvec();
    BitSet nisregs;
    for (INT i = 1; i <= gltv->get_last_idx(); i++) {
        GLT * g = gltv->get(i);
        if (g == NULL || !g->has_allocated()) { continue; }

        nis.clean();
        bool on = m_ig.get_neighbor_list(nis, GLT_id(g));
        CK_USE(on);
        UINT n = nis.get_elem_count();
        for (UINT j = nis.get_head(); n > 0; j = nis.get_next(), n--) {
            GLT * ni = m_gltm.get_glt(j);
            ASSERT0(ni);
            if (!ni->has_allocated()) { continue; }
            nisregs.clean();
            nisregs.bunion(GLT_phy(ni));
            if (GLT_rg_sz(ni) > 1) {
                ASSERT0(GLT_rg_sz(ni) == RG_PAIR_SZ);
                nisregs.bunion(GLT_phy(ni) + 1);
            }

            ASSERT(!nisregs.is_contain(GLT_phy(g)),
                    ("conflict with neighbour"));
            if (GLT_rg_sz(g) > 1) {
                ASSERT0(GLT_rg_sz(g) == RG_PAIR_SZ);
                ASSERT(!nisregs.is_contain(GLT_phy(g) + 1),
                        ("conflict with neighbour"));
            }
        }
    }

    BBList * bbl = m_ru->get_bb_list();
    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
        LTMgr * ltm = m_gltm.map_bb2ltm(bb);
        if (ltm == NULL) { continue; }
        Vector<LT*> * lvec = ltm->get_lt_vec();
        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
            LT * l = lvec->get(i);
            if (l == NULL || !l->has_allocated()) { continue; }

            IG * ig = ltm->get_ig();
            ASSERT0(ig);
            nis.clean();
            bool on = ig->get_neighbor_list(nis, LT_uid(l));
            CK_USE(on);
            UINT n = nis.get_elem_count();
            for (UINT j = nis.get_head(); n > 0; j = nis.get_next(), n--) {
                LT * ni = ltm->get_lt(j);
                ASSERT0(ni);
                if (!ni->has_allocated()) { continue; }
                nisregs.clean();
                nisregs.bunion(LT_phy(ni));
                if (LT_rg_sz(ni) > 1) {
                    ASSERT0(LT_rg_sz(ni) == RG_PAIR_SZ);
                    nisregs.bunion(LT_phy(ni) + 1);
                }

                ASSERT(!nisregs.is_contain(LT_phy(l)),
                        ("conflict with neighbour"));
                if (LT_rg_sz(l) > 1) {
                    ASSERT0(LT_rg_sz(l) == RG_PAIR_SZ);
                    ASSERT(!nisregs.is_contain(LT_phy(l) + 1),
                            ("conflict with neighbour"));
                }
            }
        }
    }
    return true;
}


//Verify the phy of lt in group are continuous.
bool RA::verify_ltg()
{
    TMapIter<UINT, LTG*> iter;
    LTG * ltg;
    for (UINT id = m_gltm.m_ltgmgr.get_first(iter, &ltg);
         id != 0; id = m_gltm.m_ltgmgr.get_next(iter, &ltg)) {
        IR * ir = m_ru->get_ir(id);
        ASSERT0(ir && is_range_call(ir) && ltg);

        IR * arg = NULL;
        for (arg = CALL_param_list(ir); arg != NULL; arg = IR_next(arg)) {
            if (arg->is_pr()) { break; }
        }

        ASSERT(arg, ("No PR parameter exist."));

        UINT phy = 0;
        for (INT i = 0; i <= ltg->get_last_idx(); i++, arg = IR_next(arg)) {
            ASSERT(arg, ("%dth parameter not exist.", i));
            ASSERT(arg->is_pr(), ("%dth parameter must be PR.", i));
            LT * l = ltg->get(i);
            ASSERT0(l && l->has_allocated());
            if (i == 0) {
                phy = LT_phy(l);
            } else {
                ASSERT(LT_phy(l) == phy, ("phy of ltg must be continuous"));
            }

            phy += LT_rg_sz(l);

            ASSERT0(arg);
            UINT bsz = arg->get_dtype_size(m_tm);
            UNUSED(bsz);
            if (LT_rg_sz(l) == 1) {
                ASSERT0(bsz <= BYTE_PER_INT);
            } else if (LT_rg_sz(l) == 2) {
                ASSERT(bsz == BYTE_PER_LONGLONG, ("lt should not be pair"));
            } else {
                UNREACH();
            }
        }
    }
    return true;
}


//'nis': for tmp used.
void RA::allocGlobal(List<UINT> & nis, List<UINT> & nis2)
{
    List<GLT*> prios;
    List<GLT*> unalloc; //Record uncolored life times.
    for (;;) {
        prios.clean();
        buildPrioList(prios);

        unalloc.clean();
        allocPrioList(prios, unalloc, nis, nis2);
        if (unalloc.get_elem_count() == 0) {
            return;
        }
        solveConflict(unalloc, nis);
    }
}


bool RA::perform(OptCtx & oc)
{
    bool omit_constrain = true;
    //Antutu4.4.1
    //if (strcmp(name, "Lorg/a/a/h;::a")==0) {
    //if (strcmp(name, "Landroid/support/v4/view/ViewPager;::a")==0) {
    //if (strcmp(name, "Landroid/support/v4/app/aa;::onItemClick") == 0) {
    //if (strcmp(name, "Lcom/antutu/benchmark/view/CircleProgress;::onDraw") == 0) {
    //if (strcmp(name, "Lcom/xiaomi/network/HostManager;::generateHostStats") == 0) {
    //if (strcmp(name, "Lorg/a/a/h;::a") == 0) { //count == 3

    //Antutu5.1
    //if (strcmp(name, "Lcom/antutu/benchmark/f/v;::a") == 0) { //hot
    //if (strcmp(name, "Lcom/antutu/benchmark/f/v;::b") == 0) { //hot
    //if (strcmp(name, "Lcom/a/a/aq;::handleMessage") == 0) { //maxvar must be
    //if (strcmp(name, "La/d;::a") == 0) { //verify_interf failed

    //Linpack.
    //if (strcmp(name, "Lcom/adwhirl/AdWhirlManager;::fetchConfig") == 0) { //paired bug
    //if (strcmp(name, "Lcom/greenecomputing/linpack/Linpack;::dgesl") == 0) { //paired bug
    ///if (strcmp(name, "Lcom/flurry/android/q;::a") == 0) { //bug, fixed
    //if (strcmp(name, "Lcom/greenecomputing/linpack/Linpack;::dgefa") == 0) { //bug, fixed
    //if (strcmp(name, "Lcom/millennialmedia/android/VideoPlayer$VideoServer;::run") == 0) { //bug, fixed
    //if (strcmp(name, "Lcom/adwhirl/adapters/CustomAdapter;::displayCustom") == 0) { //insert too many move.
    //if (strcmp(name, "Lcom/google/gson/JsonArrayDeserializationVisitor;::<init>") == 0) { //bug, fixed
    //if (strcmp(name, "Lcom/flurry/android/CatalogActivity;::onCreate") == 0) { //insert too many move.
    //if (strcmp(name, "Lcom/admob/android/ads/AdContainer;::drawText") == 0) { //phy exceed usable set.
    if (m_ru->isRegionName("Lcom/admob/android/ads/AdRequester;::requestAd")) { //glt interference.
        gcount++;
        //if (gcount==1)
        {
            gdebug();
        }
    }
    get_glt(10); //for debug symbol
    get_lt(1,0); //for debug symbol
    dumpBBList(m_ru->get_bb_list(), m_ru);
    m_cfg->dump_vcg();
    m_pr2v->dump();
    START_TIMER("GRA");

    ASSERT0(m_var2pr);
    m_prdf.setVAR2PR(m_var2pr);
    m_prdf.perform(oc);
    m_cfg->computeExitList();
    m_gltm.set_consider_local_interf(true);
    m_gltm.build(false);

    //The new info collected by glt should reflect back to local part.
    updateLocal();
    ASSERT0(verify_glt(false) && verify_lt_occ());

    m_rsc.perform(omit_constrain);
    ASSERT0(verify_usable());

    m_ig.set_consider_local_interf(true);
    m_ig.build();
    buildLocalIG();

    allocParameter();
    allocGroup();
    ASSERT0(verify_ltg() && verify_usable());
    ASSERT0(verify_reg(!omit_constrain, false));

    List<UINT> nis, nis2;
    if (!omit_constrain) {
        allocLocalSpec(nis);
    }
    allocGlobal(nis, nis2);
    updateLocal(); //TODO: remove it.
    ASSERT0(verify_reg(!omit_constrain, false));
    ASSERT0(verify_glt(true));
    ASSERT0(verify_lt_occ());

    allocLocal(nis, omit_constrain);
    ASSERT0(verify_reg(!omit_constrain, true));
    ASSERT0(m_gltm.verify());
    ASSERT0(verify_glt(true) && verify_ltg());
    if (!omit_constrain) {
        ASSERT0(verify_rsc());
    }
    ASSERT0(verify_lt_occ());

    m_gltm.renameLocal();
    ASSERT0(m_gltm.verify());

    ASSERT0(verify_lt_occ());
    if (omit_constrain) {
        rotateReg();
        ASSERT0(verify_glt(true) && verify_ltg());
        reviseRSC();
        ASSERT0(verify_reg(false, true)); //Check usable reg is dispensable here.
    } else {
        reviseParam();
        ASSERT0(verify_reg(true, true));
    }
    ASSERT0(m_gltm.verify());
    ASSERT0(verify_glt(true) && verify_ltg() && verify_rsc());
    ASSERT0(verify_interf());
    m_gltm.freeGLTBitset();

    END_TIMER();
    return true;
}
//END RA
