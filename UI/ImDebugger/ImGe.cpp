#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_thin3d.h"
#include "UI/ImDebugger/ImGe.h"
#include "UI/ImDebugger/ImDebugger.h"
#include "GPU/Common/GPUDebugInterface.h"
#include "GPU/Common/FramebufferManagerCommon.h"
#include "GPU/Common/TextureCacheCommon.h"

#include "Core/HLE/sceDisplay.h"
#include "Core/HW/Display.h"
#include "GPU/Debugger/State.h"
#include "GPU/GPUState.h"

void DrawFramebuffersWindow(ImConfig &cfg, FramebufferManagerCommon *framebufferManager) {
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Framebuffers", &cfg.framebuffersOpen)) {
		ImGui::End();
		return;
	}

	framebufferManager->DrawImGuiDebug(cfg.selectedFramebuffer);

	ImGui::End();
}

void DrawTexturesWindow(ImConfig &cfg, TextureCacheCommon *textureCache) {
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Textures", &cfg.texturesOpen)) {
		ImGui::End();
		return;
	}

	textureCache->DrawImGuiDebug(cfg.selectedTexAddr);

	ImGui::End();
}

void DrawDisplayWindow(ImConfig &cfg, FramebufferManagerCommon *framebufferManager) {
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Display", &cfg.displayOpen)) {
		ImGui::End();
		return;
	}

	ImGui::Checkbox("Display latched", &cfg.displayLatched);

	PSPPointer<u8> topaddr;
	u32 linesize;
	u32 pixelFormat;

	__DisplayGetFramebuf(&topaddr, &linesize, &pixelFormat, cfg.displayLatched);

	VirtualFramebuffer *fb = framebufferManager->GetVFBAt(topaddr.ptr);
	if (fb && fb->fbo) {
		ImTextureID texId = ImGui_ImplThin3d_AddFBAsTextureTemp(fb->fbo, Draw::FB_COLOR_BIT, ImGuiPipeline::TexturedOpaque);
		ImGui::Image(texId, ImVec2(fb->width, fb->height));
		ImGui::Text("%s - %08x", fb->fbo->Tag(), topaddr.ptr);
	} else {
		// TODO: Sometimes we should display RAM here.
		ImGui::Text("Framebuffer not available to display");
	}

	ImGui::End();
}

// Note: This is not exclusively graphics.
void DrawDebugStatsWindow(ImConfig &cfg) {
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Debug Stats", &cfg.debugStatsOpen)) {
		ImGui::End();
		return;
	}
	char statbuf[4096];
	__DisplayGetDebugStats(statbuf, sizeof(statbuf));
	ImGui::TextUnformatted(statbuf);
	ImGui::End();
}

// Stub
void DrawGeDebuggerWindow(ImConfig &cfg) {
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("GE Debugger", &cfg.geDebuggerOpen)) {
		ImGui::End();
		return;
	}

	gpu->DrawImGuiDebugger();

	ImGui::End();
}

// TODO: Separate window or merge into Ge debugger?
void DrawGeStateWindow(ImConfig &cfg, GPUDebugInterface *gpuDebug) {
	ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("GE State", &cfg.geStateOpen)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("GeRegs", ImGuiTabBarFlags_None)) {
		auto buildStateTab = [&](const char *tabName, const TabStateRow *rows, size_t numRows) {
			if (ImGui::BeginTabItem(tabName)) {
				if (ImGui::BeginTable("fpr", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH)) {
					ImGui::TableSetupColumn("bkpt", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

					for (size_t i = 0; i < numRows; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("-");  // breakpoint
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(rows[i].title.data(), rows[i].title.data() + rows[i].title.size());
						ImGui::TableNextColumn();
						char temp[256];
						auto &info = rows[i];

						const bool enabled = info.enableCmd == 0 || (gstate.cmdmem[info.enableCmd] & 1) == 1;
						const u32 value = gstate.cmdmem[info.cmd] & 0xFFFFFF;
						const u32 otherValue = gstate.cmdmem[info.otherCmd] & 0xFFFFFF;
						const u32 otherValue2 = gstate.cmdmem[info.otherCmd2] & 0xFFFFFF;

						FormatStateRow(gpuDebug, temp, sizeof(temp), info.fmt, value, enabled, otherValue, otherValue2);
						ImGui::TextUnformatted(temp);
					}

					ImGui::EndTable();
				}
				ImGui::EndTabItem();
			}
		};

		buildStateTab("Flags", g_stateFlagsRows, g_stateFlagsRowsSize);
		buildStateTab("Lighting", g_stateLightingRows, g_stateLightingRowsSize);
		buildStateTab("Texture", g_stateTextureRows, g_stateTextureRowsSize);
		buildStateTab("Settings", g_stateSettingsRows, g_stateSettingsRowsSize);

		ImGui::EndTabBar();
	}
	ImGui::End();
}
