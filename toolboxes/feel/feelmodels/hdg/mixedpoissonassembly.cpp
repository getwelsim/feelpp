#include <feel/feelmodels/hdg/mixedpoisson.hpp>

#include <feel/feelmesh/complement.hpp>
#include <feel/feelalg/topetsc.hpp>
#include <feel/feelvf/blockforms.hpp>

namespace Feel
{
namespace FeelModels
{

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::solve()
{
    tic();

#ifdef USE_SAME_MAT
    auto bbf = blockform2(*M_ps, M_A_cst);
#else
    auto bbf = blockform2(*M_ps, M_A);
#endif

    auto blf = blockform1(*M_ps, M_F);

    auto U = M_ps->element();


    tic();
    Feel::cout << "Start solving" << std::endl;
    bbf.solve(_solution=U, _rhs=blf, _condense=M_useSC, _name=prefix());
    toc("MixedPoisson : static condensation");

    toc("solve");



    M_up = U(0_c);
    M_pp = U(1_c);

    for( int i = 0; i < M_integralCondition; i++ )
        M_mup[i] = U(3_c,i);

}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleAll()
{
    M_A_cst->zero();
    M_F->zero();
    this->assembleCstPart();
    this->assembleNonCstPart();
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::copyCstPart()
{
    this->setVectorToZero();

#ifndef USE_SAME_MAT
    M_A_cst->close();
    //M_A->zero();
    M_A->close();
    // copy constant parts of the matrix
    MatConvert(toPETSc(M_A_cst->getSparseMatrix())->mat(), MATSAME, MAT_INITIAL_MATRIX, &(toPETSc(M_A->getSparseMatrix())->mat()));
#endif
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::setCstMatrixToZero()
{
    M_A_cst->zero();
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::setVectorToZero()
{
    M_F->zero();
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleCstPart()
{
    auto bbf = blockform2( *M_ps, M_A_cst );
    auto u = M_Vh->element( "u" );
    auto v = M_Vh->element( "v" );
    auto p = M_Wh->element( "p" );
    auto q = M_Wh->element( "p" );
    auto w = M_Wh->element( "w" );
    auto nu = M_Ch->element( "nu" );
    auto uI = M_Ch->element( "uI" );

    auto phat = M_Mh->element( "phat" );
    auto l = M_Mh->element( "lambda" );
    auto H = M_M0h->element( "H" );
    if ( M_hFace == 0 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMax()) );
    else if ( M_hFace == 1 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMin()) );
    else if ( M_hFace == 2 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hAverage()) );
    else
        H.on( _range=elements(M_M0h->mesh()), _expr=h() );
    // stabilisation parameter
    auto tau_constant = cst(M_tauCst);

    auto sc_param = M_useSC ? 0.5 : 1.0;


    auto gammaMinusIntegral = complement(boundaryfaces(M_mesh),
                                         [this]( auto const& ewrap ) {
                                             auto const& e = unwrap_ref( ewrap );
                                             for( auto exAtMarker : this->M_IBCList)
                                             {
                                                 if ( e.hasMarker() && e.marker().value() == this->M_mesh->markerName( exAtMarker.marker() ) )
                                                     return true;
                                             }
                                             return false;
                                         });


    // -(p,div(v))_Omega
    bbf( 0_c, 1_c ) = integrate(_range=elements(M_mesh),_expr=-(idt(p)*div(v)));

    // <phat,v.n>_Gamma\Gamma_I
    bbf( 0_c, 2_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=( idt(phat)*leftface(trans(id(v))*N())+idt(phat)*rightface(trans(id(v))*N())) );
    bbf( 0_c, 2_c ) += integrate(_range=gammaMinusIntegral,
                                 _expr=idt(phat)*trans(id(v))*N());

    // (div(j),q)_Omega
    bbf( 1_c, 0_c ) += integrate(_range=elements(M_mesh), _expr=- (id(w)*divt(u)));


    // <tau p, w>_Gamma
    bbf( 1_c, 1_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=-tau_constant *
                                 ( leftfacet( pow(idv(H),M_tauOrder)*idt(p))*leftface(id(w)) +
                                   rightfacet( pow(idv(H),M_tauOrder)*idt(p))*rightface(id(w) )));
    bbf( 1_c, 1_c ) += integrate(_range=boundaryfaces(M_mesh),
                                 _expr=-(tau_constant * pow(idv(H),M_tauOrder)*id(w)*idt(p)));


    // <-tau phat, w>_Gamma\Gamma_I
    bbf( 1_c, 2_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=tau_constant * idt(phat) *
                                 ( leftface( pow(idv(H),M_tauOrder)*id(w) )+
                                   rightface( pow(idv(H),M_tauOrder)*id(w) )));
    bbf( 1_c, 2_c ) += integrate(_range=gammaMinusIntegral,
                                 _expr=tau_constant * idt(phat) * pow(idv(H),M_tauOrder)*id(w) );


    // <j.n,mu>_Omega/Gamma
    bbf( 2_c, 0_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=( id(l)*(leftfacet(trans(idt(u))*N())+
                                                rightfacet(trans(idt(u))*N())) ) );

    // <tau p, mu>_Omega/Gamma
    bbf( 2_c, 1_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=tau_constant * id(l) * ( leftfacet( pow(idv(H),M_tauOrder)*idt(p) )+
                                                                rightfacet( pow(idv(H),M_tauOrder)*idt(p) )));

    // <-tau phat, mu>_Omega/Gamma
    bbf( 2_c, 2_c ) += integrate(_range=internalfaces(M_mesh),
                                 _expr=-sc_param*tau_constant * idt(phat) * id(l) * ( leftface( pow(idv(H),M_tauOrder) )+
                                                                                      rightface( pow(idv(H),M_tauOrder) )));

    this->assembleBoundaryCond();
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleNonCstPart()
{
    this->copyCstPart();

    modelProperties().parameters().updateParameterValues();

    this->updateConductivityTerm();
    this->assembleRHS();
    this->assembleRhsBoundaryCond();

}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::updateConductivityTerm( bool isNL)
{
    auto u = M_Vh->element( "u" );
    auto v = M_Vh->element( "v" );
    auto p = M_Wh->element( "p" );
    auto w = M_Wh->element( "w" );

#ifdef USE_SAME_MAT
    auto bbf = blockform2( *M_ps, M_A_cst);
#else
    auto bbf = blockform2( *M_ps, M_A);
#endif

    for( auto const& pairMat : modelProperties().materials() )
    {
        auto marker = pairMat.first;
        auto material = pairMat.second;
        if ( !isNL )
        {
            auto cond = material.getScalar(M_conductivityKey);
            // (sigma^-1 j, v)
            bbf(0_c,0_c) += integrate(_quad=_Q<expr_order>(), _range=markedelements(M_mesh,marker),
                                      _expr=(trans(idt(u))*id(v))/cond );
        }
        else
        {
            auto cond = material.getScalar(M_nlConductivityKey, "p", idv(M_pp));
            // (sigma(p)^-1 j, v)
            bbf(0_c,0_c) += integrate(_quad=_Q<expr_order>(), _range=markedelements(M_mesh,marker),
                                      _expr=(trans(idt(u))*id(v))/cond );
        }
    }

    // (1/delta_t p, w)_Omega  [only if it is not stationary]
    if ( !this->isStationary() ) {
        bbf( 1_c, 1_c ) += integrate(_range=elements(M_mesh),
                                     _expr = -(this->timeStepBDF()->polyDerivCoefficient(0)*idt(p)*id(w)) );
    }

}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleRHS()
{
    // (p_old,w)_Omega
    if ( !this->isStationary() )
    {
        auto bdf_poly = M_bdf_mixedpoisson->polyDeriv();
        this->assemblePotentialRHS( idv(bdf_poly) , "");
    }

    auto itField = modelProperties().boundaryConditions().find( "potential");
    if ( itField != modelProperties().boundaryConditions().end() )
    {
        auto mapField = (*itField).second;
        auto itType = mapField.find( "SourceTerm" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                auto g = expr<expr_order>(exAtMarker.expression());
                if ( !this->isStationary() )
                    g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                this->assemblePotentialRHS(g, marker);
            }
        }
    }

    itField = modelProperties().boundaryConditions().find( "flux");
    if ( itField != modelProperties().boundaryConditions().end() )
    {
        auto mapField = (*itField).second;
        auto itType = mapField.find( "SourceTerm" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                auto g = expr<Dim,1,expr_order>(exAtMarker.expression());
                if ( !this->isStationary() )
                    g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                this->assembleFluxRHS(g, marker);
            }
        }
    }
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleBoundaryCond()
{
    auto itField = modelProperties().boundaryConditions().find( "potential");
    if ( itField != modelProperties().boundaryConditions().end() )
    {
        auto mapField = (*itField).second;
        auto itType = mapField.find( "Dirichlet" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                this->assembleDirichlet( marker);
            }
        }

        itType = mapField.find( "Neumann" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                this->assembleNeumann( marker);
            }
        }

        itType = mapField.find( "Neumann_exact" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                this->assembleNeumann( marker);
            }
        }

        // Need to be moved or at least check if the expression depend on t
        itType = mapField.find( "Robin" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                auto g1 = expr<expr_order>(exAtMarker.expression1());
                auto g2 = expr<expr_order>(exAtMarker.expression2());

                if ( !this->isStationary() )
                {
                    g1.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                    g2.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                }

                this->assembleRobin(g1, g2, marker);
            }
        }
    }

    for ( int i = 0; i < M_integralCondition; i++ )
        this->assembleIBC( i );

}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleRhsBoundaryCond()
{


    auto itField = modelProperties().boundaryConditions().find( "potential");
    if ( itField != modelProperties().boundaryConditions().end() )
    {
        auto mapField = (*itField).second;
        auto itType = mapField.find( "Dirichlet" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                if ( exAtMarker.isExpression() )
                {
                    auto g = expr<expr_order>(exAtMarker.expression());
                    if ( !this->isStationary() )
                        g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                    this->assembleRhsDirichlet(g, marker);
                } else if ( exAtMarker.isFile() )
                {
                    double g = 0;
                    if ( !this->isStationary() )
                    {
                        Feel::cout << "use data file to set rhs for Dirichlet BC at time " << M_bdf_mixedpoisson->time() << std::endl;
                        LOG(INFO) << "use data file to set rhs for Dirichlet BC at time " << M_bdf_mixedpoisson->time() << std::endl;

                        // data may depend on time
                        g = exAtMarker.data(M_bdf_mixedpoisson->time());
                    }
                    else
                        g = exAtMarker.data(0.1);

                    LOG(INFO) << "use g=" << g << std::endl;
                    Feel::cout << "g=" << g << std::endl;

                    this->assembleRhsDirichlet( cst(g), marker);
                }
            }
        }
        itType = mapField.find( "Neumann" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                std::string exprString = exAtMarker.expression();
                int nComp = nbComp(exprString);
                if( nComp == 1 )
                {
                    auto g = expr<expr_order>(exAtMarker.expression());
                    if ( !this->isStationary() )
                        g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                    this->assembleRhsNeumann( g, marker);
                } else if ( nComp == Dim )
                {
                    auto g = expr<Dim,1,expr_order>(exAtMarker.expression());
                    if ( !this->isStationary() )
                        g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                    /*
                     auto blf = blockform1( *M_ps, M_F );
                     auto l = M_Mh->element( "lambda" );

                     // <g_N,mu>_Gamma_N
                     blf(2_c) += integrate(_range=markedfaces(M_mesh, marker),
                     _expr=trans(g)*N() * id(l));
                     */
                    auto gn = inner(g,N());
                    this->assembleRhsNeumann( gn, marker);
                }
            }
        }

        itType = mapField.find( "Neumann_exact" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                auto p_ex = expr<expr_order>(exAtMarker.expression());
                auto gradp_ex = expr(grad<Dim>(p_ex)) ;
                if ( !this->isStationary() )
                    gradp_ex.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );

                for( auto const& pairMat : modelProperties().materials() )
                {
                    auto material = pairMat.second;
                    auto K = material.getDouble( "k" );

                    auto g = expr(-K* trans(gradp_ex)) ;
                    auto gn = inner(g,N());
                    this->assembleRhsNeumann( gn, marker);
                }
            }
        }
        /*
         itType = mapField.find( "Robin" );
         if ( itType != mapField.end() )
         {
         for ( auto const& exAtMarker : (*itType).second )
         {
         std::string marker = exAtMarker.marker();
         auto g1 = expr<expr_order>(exAtMarker.expression1());
         auto g2 = expr<expr_order>(exAtMarker.expression2());

         if ( !this->isStationary() )
         {
         g1.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
         g2.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
         }

         this->assembleRhsRobin(g1, g2, marker);
         }
         }
         */

    }


    itField = modelProperties().boundaryConditions().find( "flux");
    if ( itField != modelProperties().boundaryConditions().end() )
    {
        auto mapField = (*itField).second;
        auto itType = mapField.find( "InterfaceCondition" );
        if ( itType != mapField.end() )
        {
            for ( auto const& exAtMarker : (*itType).second )
            {
                std::string marker = exAtMarker.marker();
                auto g = expr<1,1,expr_order>(exAtMarker.expression());
                if ( !this->isStationary() )
                    g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
                Feel::cout << "Interface condition on " << marker << ":\t" << g << std::endl;
                this->assembleRhsInterfaceCondition( g, marker);
            }
        }
    }


    for ( int i = 0; i < M_integralCondition; i++ )
        this->assembleRhsIBC( i );
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleRhsIBC( int i, std::string markerOpt, double intjn )
{
    auto blf = blockform1( *M_ps, M_F );
    auto u = M_Vh->element( "u" );
    auto p = M_Wh->element( "p" );
    auto w = M_Wh->element( "w" );
    auto nu = M_Ch->element( "nu" );
    auto uI = M_Ch->element( "uI" );

    std::string marker;
    Expr<GinacEx<expr_order> > g;

    if ( !markerOpt.empty())
    {
        marker = markerOpt;
        std::ostringstream f;
        f << std::setprecision(14) << intjn;
        g = expr<expr_order>(f.str());
    }
    else
    {
        auto exAtMarker = M_IBCList[i];
        marker = exAtMarker.marker();
        if ( exAtMarker.isExpression() )
        {
            g = expr<expr_order>(exAtMarker.expression());
            if ( !this->isStationary() )
                g.setParameterValues( { {"t", M_bdf_mixedpoisson->time()} } );
        } else if ( exAtMarker.isFile() )
        {
            double d = 0;
            if ( !this->isStationary() )
            {
                Feel::cout << "use data file to set rhs for IBC at time " << M_bdf_mixedpoisson->time() << std::endl;
                LOG(INFO) << "use data file to set rhs for IBC at time " << M_bdf_mixedpoisson->time() << std::endl;

                // data may depend on time
                d = exAtMarker.data(M_bdf_mixedpoisson->time());
            }
            else
                d = exAtMarker.data(0.1);

            // Scale entries if necessary
            {
                for ( auto const& field : modelProperties().postProcess().exports().fields() )
                {
                    if ( field == "scaled_flux" )
                    {
                        for( auto const& pairMat : modelProperties().materials() )
                        {
                            auto material = pairMat.second;
                            double kk = material.getDouble( "scale_integral_file" );
                            d = kk*d;
                        }
                    }
                }
            }
            LOG(INFO) << "use g=" << d << std::endl;
            Feel::cout << "g=" << d << std::endl;

            std::ostringstream f;
            f << std::setprecision(14) << d;
            g = expr<expr_order>(f.str());
        }
    }

    double meas = integrate( _quad=_Q<expr_order>(),  _range=markedfaces(M_mesh,marker), _expr=cst(1.0)).evaluate()(0,0);

    // <I_target,m>_Gamma_I
    blf(3_c,i) += integrate( _quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker), _expr=g*id(nu)/meas );


}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleDirichlet( std::string marker)
{
    auto bbf = blockform2( *M_ps, M_A_cst);

    auto phat = M_Mh->element( "phat" );
    auto l = M_Mh->element( "lambda" );

    // <phat, mu>_Gamma_D
    bbf( 2_c, 2_c ) += integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                 _expr=idt(phat) * id(l) );
}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleNeumann( std::string marker)
{
    auto bbf = blockform2( *M_ps, M_A_cst);

    auto u = M_Vh->element( "u" );
    auto p = M_Wh->element( "p" );
    auto phat = M_Mh->element( "phat" );
    auto l = M_Mh->element( "lambda" );
    auto H = M_M0h->element( "H" );

    if ( M_hFace == 0 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMax()) );
    else if ( M_hFace == 1 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMin()) );
    else if ( M_hFace == 2 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hAverage()) );
    else
        H.on( _range=elements(M_M0h->mesh()), _expr=h() );

    // stabilisation parameter
    auto tau_constant = cst(M_tauCst);

    // <j.n,mu>_Gamma_N
    bbf( 2_c, 0_c ) += integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                 _expr=( id(l)*(trans(idt(u))*N()) ));
    // <tau p, mu>_Gamma_N
    bbf( 2_c, 1_c ) += integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                 _expr=tau_constant * id(l) * ( pow(idv(H),M_tauOrder)*idt(p) ) );
    // <-tau phat, mu>_Gamma_N
    bbf( 2_c, 2_c ) += integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                 _expr=-tau_constant * idt(phat) * id(l) * ( pow(idv(H),M_tauOrder) ) );
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::assembleIBC( int i, std::string markerOpt )
{

    auto bbf = blockform2( *M_ps, M_A_cst);

    auto u = M_Vh->element( "u" );
    auto p = M_Wh->element( "p" );
    auto w = M_Wh->element( "w" );
    auto nu = M_Ch->element( "nu" );
    auto uI = M_Ch->element( "uI" );

    auto H = M_M0h->element( "H" );


    if ( ioption(prefixvm(this->prefix(), "hface") ) == 0 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMax()) );
    else if ( ioption(prefixvm(this->prefix(), "hface") ) == 1 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hMin()) );
    else if ( ioption(prefixvm(this->prefix(), "hface") ) == 2 )
        H.on( _range=elements(M_M0h->mesh()), _expr=cst(M_Vh->mesh()->hAverage()) );
    else
        H.on( _range=elements(M_M0h->mesh()), _expr=h() );

    // stabilisation parameter
    auto tau_constant = cst(M_tauCst);

    std::string marker;
    auto exAtMarker = M_IBCList[i];

    if ( !markerOpt.empty())
    {
        marker = markerOpt;
    }
    else
    {
        marker = exAtMarker.marker();
    }

    // Feel::cout << "Matrix marker integral: " << marker << " with line " << i << std::endl;


    // <lambda, v.n>_Gamma_I
    bbf( 0_c, 3_c, 0, i ) += integrate( _range=markedfaces(M_mesh,marker),
                                        _expr= idt(uI) * (trans(id(u))*N()) );

    // <lambda, tau w>_Gamma_I
    bbf( 1_c, 3_c, 1, i ) += integrate( _range=markedfaces(M_mesh,marker),
                                        _expr=tau_constant * idt(uI) * id(w) * ( pow(idv(H),M_tauOrder)) );

    // <j.n, m>_Gamma_I
    bbf( 3_c, 0_c, i, 0 ) += integrate( _range=markedfaces(M_mesh,marker), _expr=(trans(idt(u))*N()) * id(nu) );


    // <tau p, m>_Gamma_I
    bbf( 3_c, 1_c, i, 1 ) += integrate( _range=markedfaces(M_mesh,marker),
                                        _expr=tau_constant *idt(p)  * id(nu)* ( pow(idv(H),M_tauOrder)) );

    // -<lambda2, m>_Gamma_I
    bbf( 3_c, 3_c, i, i ) += integrate( _range=markedfaces(M_mesh,marker),
                                        _expr=-tau_constant * id(nu) *idt(uI)* (pow(idv(H),M_tauOrder)) );


}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::initTimeStep()
{
    // start or restart time step scheme
    if (!this->doRestart())
    {
        // start time step
        auto itField = modelProperties().boundaryConditions().find("potential");
        if ( itField != modelProperties().boundaryConditions().end() )
        {
            auto mapField = (*itField).second;
            auto itType = mapField.find( "InitialSolution" );
            if (itType != mapField.end() )
            {
                for (auto const& exAtMarker : (*itType).second )
                {
                    if (exAtMarker.isExpression() )
                    {
                        auto p_init = expr(exAtMarker.expression() );
                        auto marker = exAtMarker.marker();

                        if ( !this->isStationary() )
                            p_init.setParameterValues( { {"t", this->time() } } );
                        M_pp = project( _space=M_Wh, _range=markedelements(M_mesh,marker), _expr=p_init );
                        if (M_integralCondition)
                        {
                            auto mup = integrate( _range = markedfaces(M_mesh,M_IBCList[0].marker()), _expr=idv(M_pp) ).evaluate()(0,0);
                            auto meas = integrate( _range = markedfaces(M_mesh,M_IBCList[0].marker()), _expr=cst(1.0) ).evaluate()(0,0);

                            Feel::cout << "Initial integral value of potential on "
                                       << M_IBCList[0].marker() << " : \t " << mup/meas << std::endl;
                        }
                    }
                }
            }
        }

        M_bdf_mixedpoisson -> start( M_pp );
        // up current time
        this->updateTime( M_bdf_mixedpoisson -> time() );
    }
    else
    {
        // start time step
        M_bdf_mixedpoisson->restart();
        // load a previous solution as current solution
        M_pp = M_bdf_mixedpoisson->unknown(0);
        // up initial time
        this->setTimeInitial( M_bdf_mixedpoisson->timeInitial() );
        // restart exporter
        //this->restartPostProcess();
        // up current time
        this->updateTime( M_bdf_mixedpoisson->time() );

        this->log("MixedPoisson","initTimeStep", "restart bdf/exporter done" );
    }

}


MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void MIXEDPOISSON_CLASS_TEMPLATE_TYPE::updateTimeStepBDF()
{
    this->log("MixedPoisson","updateTimeStepBDF", "start" );
    this->timerTool("TimeStepping").setAdditionalParameter("time",this->currentTime());
    this->timerTool("TimeStepping").start();

    int previousTimeOrder = this->timeStepBDF()->timeOrder();

    M_bdf_mixedpoisson->next( M_pp );

    int currentTimeOrder = this->timeStepBDF()->timeOrder();

    this->updateTime( M_bdf_mixedpoisson->time() );


    this->timerTool("TimeStepping").stop("updateBdf");
    if ( this->scalabilitySave() ) this->timerTool("TimeStepping").save();
    this->log("MixedPoisson","updateTimeStepBDF", "finish" );
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::createTimeDiscretization()
{
    this->log("MixedPoisson","createTimeDiscretization", "start" );
    this->timerTool("Constructor").start();


    std::string myFileFormat = soption(_name="ts.file-format");// without prefix
    std::string suffixName = "";
    if ( myFileFormat == "binary" )
        suffixName = (boost::format("_rank%1%_%2%")%this->worldComm().rank()%this->worldComm().size() ).str();
    M_bdf_mixedpoisson = bdf( _vm=Environment::vm(), _space=M_Wh ,
                              _name=prefixvm(this->prefix(),prefixvm(this->subPrefix(),"p"+suffixName)) ,
                              _prefix="",
                              _initial_time=this->timeInitial(),
                              _final_time=this->timeFinal(),
                              _time_step=this->timeStep(),
                              _restart=this->doRestart(),
                              _restart_path=this->restartPath(),
                              _restart_at_last_save=this->restartAtLastSave(),
                              _save=this->tsSaveInFile(), _freq=this->tsSaveFreq() );
    M_bdf_mixedpoisson->setfileFormat( myFileFormat );
    M_bdf_mixedpoisson->setPathSave( (fs::path(this->rootRepository()) /
                                      fs::path( prefixvm(this->prefix(), (boost::format("bdf_o_%1%_dt_%2%")%M_bdf_mixedpoisson->bdfOrder()%this->timeStep() ).str() ) ) ).string() );

    double tElapsed = this->timerTool("Constructor").stop("createTimeDiscr");
    this->log("MixedPoisson","createTimeDiscretization", (boost::format("finish in %1% s") %tElapsed).str() );
}

MIXEDPOISSON_CLASS_TEMPLATE_DECLARATIONS
void
MIXEDPOISSON_CLASS_TEMPLATE_TYPE::exportResults( double time, mesh_ptrtype mesh, op_interp_ptrtype Idh, opv_interp_ptrtype Idhv  )
{
    this->log("MixedPoisson","exportResults", "start");
    this->timerTool("PostProcessing").start();

    if ( M_exporter->exporterGeometry() != EXPORTER_GEOMETRY_STATIC && mesh  )
    {
        LOG(INFO) << "exporting on visualisation mesh at time " << time;
        M_exporter->step( time )->setMesh( mesh );
    }
    else if ( M_exporter->exporterGeometry() != EXPORTER_GEOMETRY_STATIC )
    {
        LOG(INFO) << "exporting on computational mesh at time " << time;
        M_exporter->step( time )->setMesh( M_mesh );
    }

    // Export computed solutions
    {
        for ( auto const& field : modelProperties().postProcess().exports().fields() )
        {
            if ( field == "flux" )
            {
                LOG(INFO) << "exporting flux at time " << time;

                M_exporter->step( time )->add(prefixvm(prefix(), "flux"), Idhv?(*Idhv)( M_up):M_up );
                if (M_integralCondition)
                {
                    double meas = 0.0;
                    double j_integral = 0;

                    for( auto exAtMarker : this->M_IBCList)
                    {
                        auto marker = exAtMarker.marker();
                        LOG(INFO) << "exporting integral flux at time "
                                  << time << " on marker " << marker;
                        j_integral = integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                               _expr=trans(idv(M_up))*N()).evaluate()(0,0);
                        meas = integrate(_quad=_Q<expr_order>(), _range=markedfaces(M_mesh,marker),
                                         _expr=cst(1.0)).evaluate()(0,0);
                        Feel::cout << "Integral flux on " << marker << ": " << j_integral << std::endl;
                    }
                    M_exporter->step( time )->add(prefixvm(prefix(), "integralFlux"), j_integral);
                    M_exporter->step( time )->add(prefixvm(prefix(), "integralVelocity"), j_integral/meas);

                }
            }
            else if (field == "scaled_flux" )
            {
                auto scaled_flux = M_Vh->element("scaled_flux");
                for( auto const& pairMat : modelProperties().materials() )
                {
                    auto marker = pairMat.first;
                    auto material = pairMat.second;
                    auto kk = material.getScalar( "scale_flux" );

                    scaled_flux.on( _range=markedelements(M_mesh,marker) , _expr= kk*idv(M_up));

                }

                M_exporter->step( time )->add(prefixvm(prefix(), "scaled_flux"), Idhv?(*Idhv)( scaled_flux ):scaled_flux );
            }
            else if ( field == "potential" )
            {
                LOG(INFO) << "exporting potential at time " << time;
                M_exporter->step( time )->add(prefixvm(prefix(), "potential"),
                                              Idh?(*Idh)(M_pp):M_pp);

                for( int i = 0; i < M_integralCondition; i++ )
                {
                    double export_mup = M_mup[i].max();

                    LOG(INFO) << "exporting IBC potential " << i << " at time "
                              << time << " value " << export_mup;

                    M_exporter->step( time )->add(prefixvm(prefix(), "cstPotential_1"), export_mup );

                    Feel::cout << "Integral value of potential(mup) on "
                               << M_IBCList[i].marker() << " : \t " << export_mup << std::endl;

                    // auto mup = integrate( _range = markedfaces(M_mesh,M_IBCList[i].marker()), _expr=idv(M_pp) ).evaluate()(0,0);
                    // auto meas = integrate( _range = markedfaces(M_mesh,M_IBCList[i].marker()), _expr=cst(1.0) ).evaluate()(0,0);
                    //Feel::cout << "Integral value of potential(from pp) on " << M_IBCList[i].marker() << " : \t " << mup/meas << std::endl;

                }
                auto itField = modelProperties().boundaryConditions().find("Exact solution");
                if ( itField != modelProperties().boundaryConditions().end() )
                {
                    auto mapField = (*itField).second;
                    auto itType = mapField.find( "p_exact" );
                    if (itType != mapField.end() )
                    {
                        for (auto const& exAtMarker : (*itType).second )
                        {
                            if (exAtMarker.isExpression() )
                            {
                                auto p_exact = expr(exAtMarker.expression()) ;
                                if ( !this->isStationary() )
                                    p_exact.setParameterValues( { {"t", time } } );
                                double K = 1;
                                for( auto const& pairMat : modelProperties().materials() )
                                {
                                    auto material = pairMat.second;
                                    K = material.getDouble( "k" );
                                }
                                auto gradp_exact = grad<Dim>(p_exact) ;
                                if ( !this->isStationary() )
                                    gradp_exact.setParameterValues( { {"t", time } } );
                                auto u_exact = cst(-K)*trans(gradp_exact);//expr(-K* trans(gradp_exact)) ;

                                auto p_exactExport = project( _space=M_Wh, _range=elements(M_mesh), _expr=p_exact );
                                auto u_exactExport = project( _space=M_Vh, _range=elements(M_mesh), _expr=u_exact );

                                M_exporter->step( time )->add(prefixvm(prefix(), "p_exact"), p_exactExport );
                                M_exporter->step( time )->add(prefixvm(prefix(), "u_exact"), u_exactExport );

                                // auto l2err_u = normL2( _range=elements(M_mesh), _expr= idv(M_up) - u_exact );
                                auto l2err_u = normL2( _range=elements(M_mesh), _expr= idv(M_up) - idv(u_exactExport) );
                                auto l2norm_uex = normL2( _range=elements(M_mesh), _expr= u_exact );

                                if (l2norm_uex < 1)
                                    l2norm_uex = 1.0;


                                auto l2err_p = normL2( _range=elements(M_mesh), _expr=p_exact - idv(M_pp) );
                                auto l2norm_pex = normL2( _range=elements(M_mesh), _expr=p_exact );
                                if (l2norm_pex < 1)
                                    l2norm_pex = 1.0;

                                Feel::cout << "----- Computed Errors -----" << std::endl;
                                Feel::cout << "||p-p_ex||_L2=\t" << l2err_p/l2norm_pex << std::endl;
                                Feel::cout << "||u-u_ex||_L2=\t" << l2err_u/l2norm_uex << std::endl;
                                Feel::cout << "---------------------------" << std::endl;

                                // Export the errors
                                M_exporter -> step( time )->add(prefixvm(prefix(), "p_error_L2"), l2err_p/l2norm_pex );
                                M_exporter -> step( time )->add(prefixvm(prefix(), "u_error_L2"), l2err_u/l2norm_uex );
                            }
                        }
                    }
                }
            }
            else if (field == "scaled_potential" )
            {
                auto scaled_potential = M_Wh->element("scaled_potential");
                for( auto const& pairMat : modelProperties().materials() )
                {
                    auto marker = pairMat.first;
                    auto material = pairMat.second;
                    auto kk = material.getScalar( "scale_potential" );

                    scaled_potential.on( _range=markedelements(M_mesh,marker) , _expr= kk*idv(M_pp));

                }

                M_exporter->step( time )->add(prefixvm(prefix(), "scaled_potential"), Idh?(*Idh)( scaled_potential ):scaled_potential );


                for( int i = 0; i < M_integralCondition; i++ )
                {

                    auto scaled_ibc = M_mup[i].max();
                    for( auto const& pairMat : modelProperties().materials() )
                    {
                        auto material = pairMat.second;
                        auto kk_ibc = material.getScalar( "scale_potential" ).evaluate();
                        scaled_ibc = scaled_ibc * kk_ibc;
                    }

                    LOG(INFO) << "exporting IBC scaled potential " << i << " at time "
                              << time << " value " << scaled_ibc;
                    M_exporter->step( time )->add(prefixvm(prefix(), "scaled_cstPotential_1"),
                                                  scaled_ibc );
                    Feel::cout << "Integral scaled value of potential(mup) on "
                               << M_IBCList[i].marker() << " : \t " << scaled_ibc << std::endl;

                }
            }
            else if ( field != "state variable" )
            {
                // Import data
                LOG(INFO) << "importing " << field << " at time " << time;
                double extra_export = 0.0;
                auto itField = modelProperties().boundaryConditions().find( "Other quantities");
                if ( itField != modelProperties().boundaryConditions().end() )
                {
                    auto mapField = (*itField).second;
                    auto itType = mapField.find( field );
                    if ( itType != mapField.end() )
                    {
                        for ( auto const& exAtMarker : (*itType).second )
                        {
                            if ( exAtMarker.isExpression() )
                            {
                                LOG(INFO) << "WARNING: you are trying to export a single expression";
                            }
                            else if ( exAtMarker.isFile() )
                            {
                                if ( !this->isStationary() )
                                {
                                    extra_export = exAtMarker.data(M_bdf_mixedpoisson->time());
                                }
                                else
                                    extra_export = exAtMarker.data(0.1);
                            }
                        }
                    }
                }
                // Transform data if necessary
                LOG(INFO) << "transforming " << field << "at time " << time;
                std::string field_k = field;
                field_k += "_k";
                double kk = 0.0;
                for( auto const& pairMat : modelProperties().materials() )
                {
                    auto material = pairMat.second;
                    kk = material.getDouble( field_k );
                }
                if (std::abs(kk) > 1e-10)
                    extra_export *= kk;

                // Export data
                LOG(INFO) << "exporting " << field << " at time " << time;
                M_exporter->step( time )->add(prefixvm(prefix(), field), extra_export);
            }
        }
    }


    this->timerTool("PostProcessing").stop("exportResults");
    if ( this->scalabilitySave() )
    {
        if ( !this->isStationary() )
            this->timerTool("PostProcessing").setAdditionalParameter("time",this->currentTime());
        this->timerTool("PostProcessing").save();
    }
    this->log("MixedPoisson","exportResults", "finish");
}

} // namespace FeelModels
} // namespace Feel
