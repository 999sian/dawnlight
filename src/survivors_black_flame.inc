// Black additive particles disappear. Use alpha blending only while drawing
// these owned flames, preserving native emitter callbacks and restoring the
// resource's blend mode afterwards. Never edit shared particle resources.
struct BlackFlameCallback final : JPAEmitterCallBack {
    JPAEmitterCallBack* original=nullptr;
    void bind(JPABaseEmitter* emitter) {
        if (emitter->getEmitterCallBackPtr()!=this) {
            original=emitter->getEmitterCallBackPtr();emitter->setEmitterCallBackPtr(this);
        }
    }
    void unbind(JPABaseEmitter* emitter) {
        if (emitter->getEmitterCallBackPtr()==this) emitter->setEmitterCallBackPtr(original);
        original=nullptr;
    }
    void execute(JPABaseEmitter* e) override { if(original) original->execute(e); }
    void executeAfter(JPABaseEmitter* e) override { if(original) original->executeAfter(e); }
    void draw(JPABaseEmitter* e) override {
        if(original) original->draw(e);
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);
    }
    void drawAfter(JPABaseEmitter* e) override {
        if (e->pRes) {
            const auto* shape=e->pRes->getBsp();
            GXSetBlendMode(shape->getBlendMode(),shape->getBlendSrc(),shape->getBlendDst(),shape->getLogicOp());
        }
        if(original) original->drawAfter(e);
    }
};
