#include <iostream>

#include "Frame.hpp"

int main() {
	CCTV::Frame frameStatic[] = {*CCTV::Frame::FromFile("../contrib/test/static/1.png"), *CCTV::Frame::FromFile("../contrib/test/static/2.png"), *CCTV::Frame::FromFile("../contrib/test/static/3.png"), *CCTV::Frame::FromFile("../contrib/test/static/4.png")};
	CCTV::FrameSequence seqS(frameStatic, 3, 3);
	std::cout << seqS.GetScore(std::nullopt) << std::endl;

	CCTV::Frame frameDynamic[] = {*CCTV::Frame::FromFile("../contrib/test/dynamic/1.png"), *CCTV::Frame::FromFile("../contrib/test/dynamic/2.png"), *CCTV::Frame::FromFile("../contrib/test/dynamic/3.png"), *CCTV::Frame::FromFile("../contrib/test/dynamic/4.png")};
	CCTV::FrameSequence seqD(frameDynamic, 4, 4);
	std::cout << seqD.GetScore() << std::endl;

	CCTV::Frame frameLegitimate[] = {*CCTV::Frame::FromFile("../contrib/test/legitimate/1.png"), *CCTV::Frame::FromFile("../contrib/test/legitimate/2.png"), *CCTV::Frame::FromFile("../contrib/test/legitimate/3.png"), *CCTV::Frame::FromFile("../contrib/test/legitimate/4.png")};
	CCTV::FrameSequence seqL(frameLegitimate, 4, 4);
	std::cout << seqL.GetScore() << std::endl;

	CCTV::Frame frameMalicious[] = {*CCTV::Frame::FromFile("../contrib/test/malicious/1.png"), *CCTV::Frame::FromFile("../contrib/test/malicious/2.png"), *CCTV::Frame::FromFile("../contrib/test/malicious/3.png"), *CCTV::Frame::FromFile("../contrib/test/malicious/4.png")};
	CCTV::FrameSequence seqM(frameMalicious, 4, 4);
	std::cout << seqM.GetScore() << std::endl;

	CCTV::FrameSequence seqExplosion(43);
	for (int i = 45; i < 45+45; i++) {
		std::string res = std::to_string(i);
		seqExplosion.append(*CCTV::Frame::FromFile("../contrib/test/explosion/" + res + ".png"));
	}
	std::cout << seqExplosion.GetScore() / 43 << std::endl;
	return 0;
}