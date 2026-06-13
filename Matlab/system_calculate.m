% 本程序用于求解LQR反馈矩阵lqr_k(L0)
% 对于每一个腿长L0，求解一次系统状态空间方程，然后求得反馈矩阵K
% 对于不同的K，对L0进行拟合，得到lqr_k
clear;
clc;
L0s = 0.04:0.01:0.14;       %L0变化范围
Ks =zeros(2,6,length(L0s)); %不同工况下的反馈增益矩阵K

for step=1:length(L0s)
%theta : 摆杆与竖直方向夹角             R   ：驱动轮半径
%x     : 驱动轮位移                     L   : 摆杆重心到驱动轮轴距离
%phi   : 机体与水平夹角                 LM  : 摆杆重心到机体转轴距离
%T     ：驱动轮输出力矩                 l   : 机体重心到其转轴距离
%Tp    : 髋关节输出力矩                 mw  : 驱动轮转子质量
%N     ：驱动轮对摆杆力的水平分量        mp  : 摆杆质量
%P     ：驱动轮对摆杆力的竖直分量        M   : 机体质量
%NM    ：摆杆对机体力水平方向分量        Iw  : 驱动轮转子转动惯量
%PM    ：摆杆对机体力竖直方向分量        Ip  : 摆杆绕质心转动惯量
%Nf    : 地面对驱动轮摩擦力             Im  : 机体绕质心转动惯量
    syms theta d_theta dd_theta;        
    syms x d_x dd_x;
    syms phi d_phi dd_phi;
    syms T Tp N P NM PM Nf t;
    L = L0s(step)/2;        %摆杆重心到驱动轮轴距离
    LM = L0s(step)/2;       %摆杆重心到机体转轴距离
    l = 0;                  %机体重心到其转轴距离
    g=9.8;

    R = 0.0325;             %驱动轮半径   直径65MM
    mw=0.0841;              %驱动轮质量
    mp=0.0562;              %杆质量    两个大腿两个小腿
    M=0.35305;              %机体质量  注意是实际机体的一半 313 没计算电池重量

    Iw = 4.44153E-05;       %驱动轮转动惯量
    Ip=3.4633333E-05;       %按照棱柱模型进行计算
    Im=0.00036776;         %按照棱柱模型进行计算


    %中间变量NM,PM,N,P
    NM=M*(dd_x+(L+LM)*(dd_theta*cos(theta)-d_theta^2*sin(theta))-l*(dd_phi*cos(phi)-d_phi^2*sin(phi)));
    PM=M*g-M*((L+LM)*(dd_theta*sin(theta)+d_theta^2*cos(theta))+l*(d_phi^2*cos(phi)+dd_phi*sin(phi)));
    N = NM+mp*(dd_x+L*(dd_theta*cos(theta)-d_theta^2*sin(theta)));
    P = PM+mp*g-mp*L*(d_theta^2*cos(theta)+dd_theta*sin(theta));

    %驱动轮
    eqn1 = dd_x == (T-N*R)/(Iw/R+mw*R);
    %摆杆
    eqn2 = Ip*dd_theta == (P*L+PM*LM)*sin(theta)-(N*L+NM*LM)*cos(theta)-T+Tp;
    %机体
    eqn3 = Im*dd_phi == Tp +NM*l*cos(phi)+PM*l*sin(phi);
    %得到系统非线性模型符号表达式
    [dd_x,dd_theta,dd_phi] = solve(eqn1,eqn2,eqn3,dd_x,dd_theta,dd_phi);

    %状态向量X,控制向量U，状态方程F
    X = [theta,d_theta,x,d_x,phi,d_phi];
    U = [T,Tp];
    F = [d_theta,dd_theta,d_x,dd_x,d_phi,dd_phi];

    Ja=jacobian(F,X);
    Jb=jacobian(F,U);

    A=vpa(subs(Ja,X,[0 0 0 0 0 0]));
    B=vpa(subs(Jb,X,[0 0 0 0 0 0]));
    % 离散化
    [G,H]=c2d(eval(A),eval(B),0.005);
    
    % 定义权重矩阵Q, R
    %   Q=diag([1 10 100 20 1000 1]);
    Q=diag([1 10 400 100 500 1]);
    R=diag([1 1]);

    % 求解反馈矩阵K
    Ks(:,:,step)=dlqr(G,H,Q,R);
end
%创建符号矩阵K
K=sym('K',[2 6]);
syms L0;
for x=1:2
    for y=1:6
        p=polyfit(L0s,reshape(Ks(x,y,:),1,length(L0s)),3);
        K(x,y)=p(1)*L0^3+p(2)*L0^2+p(3)*L0+p(4);
    end
end

matlabFunction(K,'File','out_lqr_k');

vpa(subs(K,L0,0.07))
