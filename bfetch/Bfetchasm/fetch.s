.intel_syntax noprefix

# bfetchasm: static x86_64 Linux syscall-only fetcher.
# This rewrite is intentionally machine-targeted to the current box.
#
# Cost model:
#   T_total = T_startup + T_syscalls + T_bytes_scanned + T_format
# For a one-shot fetcher, the hard wall is not arithmetic, it is metadata:
# directory enumeration, procfs/sysfs formatting, and manifest scanning.
# The comments below spell each field as a small equation rather than pretending
# the runtime is some abstract "constant time".

.equ SYS_read,         0
.equ SYS_write,        1
.equ SYS_open,         2
.equ SYS_close,        3
.equ SYS_uname,       63
.equ SYS_getppid,    110
.equ SYS_getdents64, 217
.equ SYS_readlinkat, 267
.equ SYS_openat,     257
.equ SYS_exit,        60

.equ AT_FDCWD,      -100
.equ O_RDONLY,         0
.equ O_DIRECTORY,  0x10000
.equ O_CLOEXEC,    0x80000

.equ UTS_RELEASE_OFF, 130
.equ D_RECLEN_OFF,    16
.equ D_NAME_OFF,      19

.equ SMALL_BUF,      256
.equ FILE_BUF,    262144
.equ DIR_BUF,     131072
.equ OUT_BUF,      16384
.equ PATH_BUF,       512

.section .rodata
version_str:             .asciz "bfetchasm version 0.2.0\n"
help_str:
    .asciz "Usage: bfetchasm [-v|--version] [-h|--help]\n"

unknown_str:             .asciz "Unknown"
unknown_gpu_str:         .asciz "Unknown GPU"
linux_str:               .asciz "Linux"
slash_str:               .asciz "/"

path_os_release:         .asciz "/etc/os-release"
path_uptime:             .asciz "/proc/uptime"
path_meminfo:            .asciz "/proc/meminfo"
path_cpu_online:         .asciz "/sys/devices/system/cpu/online"
path_cpu_present:        .asciz "/sys/devices/system/cpu/present"
path_cpu_freq:           .asciz "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
path_nvidia_gpus:        .asciz "/proc/driver/nvidia/gpus"
path_card0_uevent:       .asciz "/sys/class/drm/card0/device/uevent"
path_pacman:             .asciz "/var/lib/pacman/local"
path_flatpak_sys:        .asciz "/var/lib/flatpak/app"
path_nix_default:        .asciz "/nix/var/nix/profiles/default/manifest.json"
path_nix_system:         .asciz "/run/current-system/sw/manifest.json"

suffix_information:      .asciz "/information"
name_information:        .asciz "information"
suffix_flatpak_user:     .asciz "/.local/share/flatpak/app"
suffix_nix_profile_json: .asciz "/.nix-profile/manifest.json"
suffix_nix_profile_nix:  .asciz "/.nix-profile/manifest.nix"
suffix_nix_home_mgr:     .asciz "/.local/state/nix/profiles/home-manager/manifest.json"

needle_pretty:           .asciz "PRETTY_NAME=\""
needle_model:            .asciz "Model:"
needle_driver:           .asciz "DRIVER="
needle_memtotal:         .asciz "MemTotal:"
needle_memavail:         .asciz "MemAvailable:"
needle_active:           .asciz "\"active\":true"
needle_nameeq:           .asciz "name = \""

arg_v_short:             .asciz "-v"
arg_v_long:              .asciz "--version"
arg_h_short:             .asciz "-h"
arg_h_long:              .asciz "--help"

env_home_key:            .asciz "HOME="
env_shell_key:           .asciz "SHELL="
env_termprog_key:        .asciz "TERM_PROGRAM="
env_term_key:            .asciz "TERM="
env_xdg_key:             .asciz "XDG_CURRENT_DESKTOP="
env_dsess_key:           .asciz "DESKTOP_SESSION="

shell_bash:              .asciz "bash"
shell_zsh:               .asciz "zsh"
shell_fish:              .asciz "fish"
shell_sh:                .asciz "sh"
shell_sudo:              .asciz "sudo"

proc_prefix:             .asciz "/proc/"
proc_exe_suffix:         .asciz "/exe"
proc_stat_suffix:        .asciz "/stat"

cpu_kw1:                 .asciz " with Radeon Graphics"
cpu_kw2:                 .asciz " 6-Core"
cpu_kw3:                 .asciz " 8-Core"
cpu_kw4:                 .asciz " 12-Core"
cpu_kw5:                 .asciz " 16-Core"
cpu_kw6:                 .asciz " 24-Core"
cpu_kw7:                 .asciz " 32-Core"
cpu_kw8:                 .asciz " Processor"

pkg_pacman_lbl:          .asciz " (pacman), "
pkg_flatpak_lbl:         .asciz " (flatpak), "
pkg_nix_lbl:             .asciz " (nix), "

gib_sep:                 .asciz " GiB / "
gib_tail:                .asciz " GiB"
uptime_d:                .asciz "d "
uptime_h:                .asciz "h "
uptime_m:                .asciz "m"
cpu_open_paren:          .asciz " ("
cpu_close_paren:         .asciz ")"
cpu_close_at:            .asciz ") @ "
cpu_ghz:                 .asciz " GHz"
newline_str:             .asciz "\n"

art0:  .asciz " ┌──┐ ┌──────────────────────────────────┐ ┌────┐\n"
art1:  .asciz " │▒▒│ │─────/--++++++++++----/───────────│ │ 境 │\n"
art2:  .asciz " │██│ │────//+++++++++++----/─────/\\\\────│ │    │\n"
art3:  .asciz " │██│ │───//++++++++++++++++──────\\//────│ │ 界 │\n"
art4:  .asciz " │██│ │──//++---+//──────────────────────│ └────┘\n"
art5:  .asciz " │██│ │─//---+++//────────────/+\\\\───────│\n"
art6:  .asciz " │██│ │─\\\\++++--/─────────────\\-//───────│\n"
art7:  .asciz " │██│ │──\\\\--+++\\──────────────────/++\\\\─│\n"
art8:  .asciz " │██│ │───\\\\+++--\\─────────────────\\--//─│\n"
art9:  .asciz " │██│ │────\\\\--++++-+---+--++++++/───────│\n"
art10: .asciz " │██│ │─────\\--+++++++++++++++--/────────│\n"
art11: .asciz " │██│ │──────\\-++++++++++++----/─────────│\n"
art12: .asciz " │██│ └──────────────────────────────────┘\n"
art13: .asciz " └──┘\n"

line_distro:             .asciz " │██│ Distro: "
line_kernel:             .asciz " │██│ Kernel: "
line_uptime:             .asciz " │██│ Uptime: "
line_wm:                 .asciz " │██│ WM: "
line_packages:           .asciz " │██│ Packages: "
line_terminal:           .asciz " │██│ Terminal: "
line_memory:             .asciz " │██│ Memory: "
line_shell:              .asciz " │██│ Shell: "
line_cpu:                .asciz " │██│ CPU: "
line_gpu:                .asciz " │▒▒│ GPU: "

.section .bss
.balign 8
env_home:                .quad 0
env_shell:               .quad 0
env_termprog:            .quad 0
env_term:                .quad 0
env_xdg:                 .quad 0
env_dsess:               .quad 0
out_off:                 .quad 0

distro_buf:              .skip SMALL_BUF
kernel_buf:              .skip SMALL_BUF
uptime_buf:              .skip SMALL_BUF
wm_buf:                  .skip SMALL_BUF
term_buf:                .skip SMALL_BUF
shell_buf:               .skip SMALL_BUF
cpu_buf:                 .skip SMALL_BUF
gpu_buf:                 .skip SMALL_BUF
pkg_buf:                 .skip SMALL_BUF
mem_buf:                 .skip SMALL_BUF

brand_buf:               .skip 96
brand_clean:             .skip 96
small_buf:               .skip 4096
file_buf:                .skip FILE_BUF
dir_buf:                 .skip DIR_BUF
out_buf:                 .skip OUT_BUF
tmp_path:                .skip PATH_BUF
tmp_num:                 .skip 64
uts_buf:                 .skip 390

.section .text
.global _start

_start:
    cld
    mov r12, rsp
    mov r13, [rsp]               # argc
    lea r14, [rsp + 8]           # argv
    lea r15, [r14 + r13*8 + 8]   # envp

    mov rdi, r13
    mov rsi, r14
    call parse_args

    mov rdi, r15
    call init_env_cache

    lea rdi, [rip + distro_buf]
    call get_distro
    lea rdi, [rip + kernel_buf]
    call get_kernel
    lea rdi, [rip + uptime_buf]
    call get_uptime
    lea rdi, [rip + mem_buf]
    call get_memory
    lea rdi, [rip + wm_buf]
    call get_wm
    lea rdi, [rip + term_buf]
    call get_terminal
    lea rdi, [rip + shell_buf]
    call get_shell
    lea rdi, [rip + cpu_buf]
    call get_cpu
    lea rdi, [rip + gpu_buf]
    call get_gpu
    lea rdi, [rip + pkg_buf]
    call get_packages
    call build_output

    mov eax, SYS_write
    mov edi, 1
    lea rsi, [rip + out_buf]
    mov rdx, [rip + out_off]
    syscall

    xor edi, edi
    call exit_now

parse_args:
    cmp rdi, 1
    jle .parse_args_ret
    mov rcx, 1
.parse_args_loop:
    cmp rcx, rdi
    jge .parse_args_ret
    mov rbx, [rsi + rcx*8]

    mov rdi, rbx
    lea rsi, [rip + arg_v_short]
    call streq
    test eax, eax
    jnz .parse_args_show_version

    mov rdi, rbx
    lea rsi, [rip + arg_v_long]
    call streq
    test eax, eax
    jnz .parse_args_show_version

    mov rdi, rbx
    lea rsi, [rip + arg_h_short]
    call streq
    test eax, eax
    jnz .parse_args_show_help

    mov rdi, rbx
    lea rsi, [rip + arg_h_long]
    call streq
    test eax, eax
    jnz .parse_args_show_help

    inc rcx
    jmp .parse_args_loop
.parse_args_show_version:
    lea rdi, [rip + version_str]
    call print_and_exit
.parse_args_show_help:
    lea rdi, [rip + help_str]
    call print_and_exit
.parse_args_ret:
    ret

print_and_exit:
    push rdi
    call cstr_len
    mov rdx, rax
    pop rsi
    mov eax, SYS_write
    mov edi, 1
    syscall
    xor edi, edi
    call exit_now

exit_now:
    mov eax, SYS_exit
    syscall

# init_env_cache:
#   Work = O(sum of env bytes)
#   Benefit = one pass up front instead of repeated getenv scans.
init_env_cache:
    push rbx
    push r12
    mov r12, rdi
    xor ebx, ebx
.init_env_cache_env_loop:
    mov rax, [r12 + rbx*8]
    test rax, rax
    jz .init_env_cache_done
    mov r8, rax

    mov rdi, r8
    lea rsi, [rip + env_home_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_check_shell
    lea rax, [r8 + 5]
    mov [rip + env_home], rax
    jmp .init_env_cache_next

.init_env_cache_check_shell:
    mov rdi, r8
    lea rsi, [rip + env_shell_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_check_termprog
    lea rax, [r8 + 6]
    mov [rip + env_shell], rax
    jmp .init_env_cache_next

.init_env_cache_check_termprog:
    mov rdi, r8
    lea rsi, [rip + env_termprog_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_check_term
    lea rax, [r8 + 13]
    mov [rip + env_termprog], rax
    jmp .init_env_cache_next

.init_env_cache_check_term:
    mov rdi, r8
    lea rsi, [rip + env_term_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_check_xdg
    lea rax, [r8 + 5]
    mov [rip + env_term], rax
    jmp .init_env_cache_next

.init_env_cache_check_xdg:
    mov rdi, r8
    lea rsi, [rip + env_xdg_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_check_dsess
    lea rax, [r8 + 20]
    mov [rip + env_xdg], rax
    jmp .init_env_cache_next

.init_env_cache_check_dsess:
    mov rdi, r8
    lea rsi, [rip + env_dsess_key]
    call has_prefix
    test eax, eax
    jz .init_env_cache_next
    lea rax, [r8 + 16]
    mov [rip + env_dsess], rax

.init_env_cache_next:
    inc rbx
    jmp .init_env_cache_env_loop
.init_env_cache_done:
    pop r12
    pop rbx
    ret

streq:
    xor eax, eax
.streq_loop:
    mov dl, byte ptr [rdi]
    mov cl, byte ptr [rsi]
    cmp dl, cl
    jne .streq_ret
    test dl, dl
    jz .streq_yes
    inc rdi
    inc rsi
    jmp .streq_loop
.streq_yes:
    mov eax, 1
.streq_ret:
    ret

has_prefix:
    xor eax, eax
.has_prefix_loop:
    mov dl, byte ptr [rsi]
    test dl, dl
    jz .has_prefix_yes
    mov cl, byte ptr [rdi]
    cmp cl, dl
    jne .has_prefix_ret
    inc rdi
    inc rsi
    jmp .has_prefix_loop
.has_prefix_yes:
    mov eax, 1
.has_prefix_ret:
    ret

cstr_len:
    mov rax, rdi
.cstr_len_loop:
    cmp byte ptr [rax], 0
    je .cstr_len_done
    inc rax
    jmp .cstr_len_loop
.cstr_len_done:
    sub rax, rdi
    ret

basename_ptr:
    mov rax, rdi
    mov rcx, rdi
.basename_ptr_loop:
    mov dl, byte ptr [rcx]
    test dl, dl
    jz .basename_ptr_ret
    cmp dl, '/'
    jne .basename_ptr_next
    lea rax, [rcx + 1]
.basename_ptr_next:
    inc rcx
    jmp .basename_ptr_loop
.basename_ptr_ret:
    ret

find_substr:
    push rbx
    push r12
    mov rbx, rdi
    mov r12, rsi
    mov rdi, r12
    call cstr_len
    mov r8, rax
    test r8, r8
    jz .find_substr_none
.find_substr_outer:
    mov al, byte ptr [rbx]
    test al, al
    jz .find_substr_none
    xor rcx, rcx
.find_substr_inner:
    cmp rcx, r8
    je .find_substr_found
    mov al, byte ptr [rbx + rcx]
    test al, al
    jz .find_substr_none
    cmp al, byte ptr [r12 + rcx]
    jne .find_substr_next_outer
    inc rcx
    jmp .find_substr_inner
.find_substr_next_outer:
    inc rbx
    jmp .find_substr_outer
.find_substr_found:
    mov rax, rbx
    pop r12
    pop rbx
    ret
.find_substr_none:
    xor rax, rax
    pop r12
    pop rbx
    ret

copy_cstr_cap:
    test rdx, rdx
    jz .copy_cstr_cap_ret
    dec rdx
    xor rcx, rcx
.copy_cstr_cap_loop:
    cmp rcx, rdx
    jae .copy_cstr_cap_term
    mov al, byte ptr [rsi + rcx]
    mov byte ptr [rdi + rcx], al
    test al, al
    jz .copy_cstr_cap_ret
    inc rcx
    jmp .copy_cstr_cap_loop
.copy_cstr_cap_term:
    mov byte ptr [rdi + rcx], 0
.copy_cstr_cap_ret:
    ret

append_cap:
    test rdx, rdx
    jz .append_cap_ret
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx

    mov rdi, r8
    call cstr_len
    mov r11, rax

    mov rdi, r9
    call cstr_len
    mov rcx, rax

    mov rax, r10
    dec rax
    cmp r11, rax
    jae .append_cap_ret
    sub rax, r11
    cmp rcx, rax
    jbe .append_cap_fit
    mov rcx, rax
.append_cap_fit:
    lea rdi, [r8 + r11]
    mov rsi, r9
    rep movsb
    mov byte ptr [rdi], 0
.append_cap_ret:
    ret

append_into_field:
    mov edx, SMALL_BUF
    jmp append_cap

append_u64_field:
    push rbx
    mov rbx, rdi
    mov rdi, rsi
    lea rsi, [rip + tmp_num]
    call u64_to_cstr
    mov rdi, rbx
    lea rsi, [rip + tmp_num]
    call append_into_field
    pop rbx
    ret

append_fixed2_field:
    push rbx
    push r12
    mov r12, rdi
    mov rax, rsi
    xor edx, edx
    mov ecx, 100
    div rcx
    mov rbx, rdx

    mov rdi, rax
    lea rsi, [rip + tmp_num]
    call u64_to_cstr
    mov rdi, r12
    lea rsi, [rip + tmp_num]
    call append_into_field

    mov rax, rbx
    xor edx, edx
    mov ecx, 10
    div rcx
    add al, '0'
    add dl, '0'
    mov byte ptr [rip + tmp_num], '.'
    mov byte ptr [rip + tmp_num + 1], al
    mov byte ptr [rip + tmp_num + 2], dl
    mov byte ptr [rip + tmp_num + 3], 0
    mov rdi, r12
    lea rsi, [rip + tmp_num]
    call append_into_field
    pop r12
    pop rbx
    ret

read_file:
    push rbx
    push r12
    cmp rdx, 2
    jb .read_file_fail
    mov rbx, rsi
    mov r12, rdx
    mov eax, SYS_open
    mov rsi, O_RDONLY | O_CLOEXEC
    xor edx, edx
    syscall
    test rax, rax
    js .read_file_fail
    mov r8, rax
    mov eax, SYS_read
    mov edi, r8d
    mov rsi, rbx
    mov rdx, r12
    dec rdx
    syscall
    test rax, rax
    jl .read_file_close_fail
    mov byte ptr [rbx + rax], 0
    mov r9, rax
    mov eax, SYS_close
    mov edi, r8d
    syscall
    mov rax, r9
    pop r12
    pop rbx
    ret
.read_file_close_fail:
    mov eax, SYS_close
    mov edi, r8d
    syscall
.read_file_fail:
    xor rax, rax
    pop r12
    pop rbx
    ret

u64_to_cstr:
    push rbx
    mov rax, rdi
    mov rbx, rsi
    test rax, rax
    jnz .u64_to_cstr_conv
    mov byte ptr [rbx], '0'
    mov byte ptr [rbx + 1], 0
    mov rax, rbx
    pop rbx
    ret
.u64_to_cstr_conv:
    lea rcx, [rbx + 31]
    mov byte ptr [rcx], 0
.u64_to_cstr_loop:
    xor edx, edx
    mov r8, 10
    div r8
    add dl, '0'
    dec rcx
    mov byte ptr [rcx], dl
    test rax, rax
    jnz .u64_to_cstr_loop
    mov rdi, rbx
    mov rsi, rcx
    mov edx, 32
    call copy_cstr_cap
    mov rax, rbx
    pop rbx
    ret

parse_u64_dec:
    xor rax, rax
.parse_u64_dec_loop:
    movzx rcx, byte ptr [rdi]
    cmp rcx, '0'
    jb .parse_u64_dec_ret
    cmp rcx, '9'
    ja .parse_u64_dec_ret
    imul rax, rax, 10
    sub rcx, '0'
    add rax, rcx
    inc rdi
    jmp .parse_u64_dec_loop
.parse_u64_dec_ret:
    ret

append_mem:
    push rbx
    mov r9, rdi
    mov rbx, [rip + out_off]
    cmp rbx, OUT_BUF
    jae .append_mem_ret
    mov ecx, OUT_BUF
    sub rcx, rbx
    cmp rsi, rcx
    jbe .append_mem_fit
    mov rsi, rcx
.append_mem_fit:
    mov r8, rsi
    lea rdx, [rip + out_buf]
    add rdx, rbx
    mov rdi, rdx
    mov rsi, r9
    mov rcx, r8
    rep movsb
    add qword ptr [rip + out_off], r8
.append_mem_ret:
    pop rbx
    ret

append_cstr:
    push rdi
    call cstr_len
    mov rsi, rax
    pop rdi
    jmp append_mem

out_line_value:
    push rsi
    call append_cstr
    pop rdi
    call append_cstr
    lea rdi, [rip + newline_str]
    call append_cstr
    ret

build_home_path:
    mov r8, rdi
    mov r9, rsi
    mov rsi, [rip + env_home]
    test rsi, rsi
    jz .build_home_path_fail
    mov rdi, r8
    mov edx, PATH_BUF
    call copy_cstr_cap
    mov rdi, r8
    mov rsi, r9
    mov edx, PATH_BUF
    call append_cap
    mov eax, 1
    ret
.build_home_path_fail:
    xor eax, eax
    ret

pid_to_path:
    push r12
    push r13
    push r14
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    mov rdi, r12
    lea rsi, [rip + proc_prefix]
    mov edx, PATH_BUF
    call copy_cstr_cap

    mov rdi, r13
    lea rsi, [rip + tmp_num]
    call u64_to_cstr
    mov rdi, r12
    lea rsi, [rip + tmp_num]
    mov edx, PATH_BUF
    call append_cap

    mov rdi, r12
    mov rsi, r14
    mov edx, PATH_BUF
    call append_cap

    pop r14
    pop r13
    pop r12
    ret

count_dir_entries:
    push rbx
    push r12
    push r13
    push r14
    xor r13, r13

    mov eax, SYS_open
    mov rsi, O_RDONLY | O_DIRECTORY | O_CLOEXEC
    xor edx, edx
    syscall
    test rax, rax
    js .count_dir_entries_fail
    mov r12, rax

.count_dir_entries_outer:
    mov eax, SYS_getdents64
    mov edi, r12d
    lea rsi, [rip + dir_buf]
    mov edx, DIR_BUF
    syscall
    test rax, rax
    jle .count_dir_entries_done
    mov r14, rax
    xor rbx, rbx
.count_dir_entries_inner:
    cmp rbx, r14
    jae .count_dir_entries_outer
    lea r8, [rip + dir_buf]
    add r8, rbx
    lea r9, [r8 + D_NAME_OFF]
    mov al, byte ptr [r9]
    cmp al, '.'
    jne .count_dir_entries_count
    cmp byte ptr [r9 + 1], 0
    je .count_dir_entries_skip
    cmp byte ptr [r9 + 1], '.'
    jne .count_dir_entries_count
    cmp byte ptr [r9 + 2], 0
    je .count_dir_entries_skip
.count_dir_entries_count:
    inc r13
.count_dir_entries_skip:
    movzx eax, word ptr [r8 + D_RECLEN_OFF]
    add rbx, rax
    jmp .count_dir_entries_inner
.count_dir_entries_done:
    mov eax, SYS_close
    mov edi, r12d
    syscall
    mov rax, r13
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.count_dir_entries_fail:
    xor rax, rax
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

count_manifest_needle:
    push rbx
    push r12
    push r13
    push r14
    mov r13, rsi
    lea rsi, [rip + file_buf]
    mov edx, FILE_BUF
    call read_file
    test rax, rax
    jz .count_manifest_needle_fail

    mov rdi, r13
    call cstr_len
    mov r14, rax

    lea r12, [rip + file_buf]
    xor rbx, rbx
.count_manifest_needle_loop:
    mov rdi, r12
    mov rsi, r13
    call find_substr
    test rax, rax
    jz .count_manifest_needle_done
    inc rbx
    add rax, r14
    mov r12, rax
    jmp .count_manifest_needle_loop
.count_manifest_needle_done:
    mov rax, rbx
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.count_manifest_needle_fail:
    xor rax, rax
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

is_shell_like:
    push rdi
    lea rsi, [rip + shell_bash]
    call streq
    pop rdi
    test eax, eax
    jnz .is_shell_like_yes

    push rdi
    lea rsi, [rip + shell_zsh]
    call streq
    pop rdi
    test eax, eax
    jnz .is_shell_like_yes

    push rdi
    lea rsi, [rip + shell_fish]
    call streq
    pop rdi
    test eax, eax
    jnz .is_shell_like_yes

    push rdi
    lea rsi, [rip + shell_sh]
    call streq
    pop rdi
    test eax, eax
    jnz .is_shell_like_yes

    lea rsi, [rip + shell_sudo]
    call streq
    ret
.is_shell_like_yes:
    mov eax, 1
    ret

parse_ppid_from_stat:
    mov rsi, rdi
.parse_ppid_from_stat_find_rparen:
    mov al, byte ptr [rsi]
    test al, al
    jz .parse_ppid_from_stat_fail
    cmp al, ')'
    je .parse_ppid_from_stat_after
    inc rsi
    jmp .parse_ppid_from_stat_find_rparen
.parse_ppid_from_stat_after:
    add rsi, 4
    mov rdi, rsi
    call parse_u64_dec
    ret
.parse_ppid_from_stat_fail:
    xor rax, rax
    ret

parse_cpu_range_list:
    xor r8, r8
.parse_cpu_range_list_skip_ws:
    mov al, byte ptr [rdi]
    cmp al, ','
    je .parse_cpu_range_list_sw_next
    cmp al, ' '
    je .parse_cpu_range_list_sw_next
    cmp al, 9
    je .parse_cpu_range_list_sw_next
    cmp al, 10
    je .parse_cpu_range_list_sw_next
    jmp .parse_cpu_range_list_maybe_num
.parse_cpu_range_list_sw_next:
    inc rdi
    jmp .parse_cpu_range_list_skip_ws
.parse_cpu_range_list_maybe_num:
    cmp byte ptr [rdi], '0'
    jb .parse_cpu_range_list_done
    cmp byte ptr [rdi], '9'
    ja .parse_cpu_range_list_done

    mov rsi, rdi
    call parse_u64_dec
    mov r9, rax
    mov r10, rax

.parse_cpu_range_list_advance_num:
    mov al, byte ptr [rdi]
    cmp al, '0'
    jb .parse_cpu_range_list_check_dash
    cmp al, '9'
    ja .parse_cpu_range_list_check_dash
    inc rdi
    jmp .parse_cpu_range_list_advance_num

.parse_cpu_range_list_check_dash:
    cmp byte ptr [rdi], '-'
    jne .parse_cpu_range_list_add_range
    inc rdi
    mov rsi, rdi
    call parse_u64_dec
    mov r10, rax
.parse_cpu_range_list_advance_num2:
    mov al, byte ptr [rdi]
    cmp al, '0'
    jb .parse_cpu_range_list_add_range
    cmp al, '9'
    ja .parse_cpu_range_list_add_range
    inc rdi
    jmp .parse_cpu_range_list_advance_num2

.parse_cpu_range_list_add_range:
    mov rax, r10
    sub rax, r9
    inc rax
    add r8, rax
    jmp .parse_cpu_range_list_skip_ws
.parse_cpu_range_list_done:
    mov rax, r8
    ret

get_cpu_threads:
    lea rdi, [rip + path_cpu_online]
    lea rsi, [rip + small_buf]
    mov edx, 128
    call read_file
    test rax, rax
    jz .get_cpu_threads_present
    lea rdi, [rip + small_buf]
    call parse_cpu_range_list
    test rax, rax
    jnz .get_cpu_threads_ret
.get_cpu_threads_present:
    lea rdi, [rip + path_cpu_present]
    lea rsi, [rip + small_buf]
    mov edx, 128
    call read_file
    test rax, rax
    jz .get_cpu_threads_one
    lea rdi, [rip + small_buf]
    call parse_cpu_range_list
    test rax, rax
    jnz .get_cpu_threads_ret
.get_cpu_threads_one:
    mov eax, 1
.get_cpu_threads_ret:
    ret

strip_keyword:
    push rdi
    call find_substr
    pop rdi
    test rax, rax
    jz .strip_keyword_ret
    mov byte ptr [rax], 0
.strip_keyword_ret:
    ret

# get_distro:
#   T = open(/etc/os-release) + read(<=4 KiB) + parse bytes once.
#   Arithmetic cost is negligible; the whole term is metadata fetch.
get_distro:
    push r12
    mov r12, rdi
    mov byte ptr [r12], 0
    lea rdi, [rip + path_os_release]
    lea rsi, [rip + small_buf]
    mov edx, 4096
    call read_file
    test rax, rax
    jz .get_distro_fallback

    lea rsi, [rip + small_buf]
    mov rdi, rsi
    lea rsi, [rip + needle_pretty]
    call find_substr
    test rax, rax
    jz .get_distro_fallback
    add rax, 13
    mov rsi, rax
    xor rcx, rcx
.get_distro_copy:
    cmp rcx, SMALL_BUF - 1
    jae .get_distro_done
    mov al, byte ptr [rsi + rcx]
    test al, al
    jz .get_distro_done
    cmp al, '"'
    je .get_distro_done
    mov byte ptr [r12 + rcx], al
    inc rcx
    jmp .get_distro_copy
.get_distro_done:
    mov byte ptr [r12 + rcx], 0
    pop r12
    ret
.get_distro_fallback:
    mov rdi, r12
    lea rsi, [rip + linux_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
    pop r12
    ret

get_kernel:
    mov r8, rdi
    mov eax, SYS_uname
    lea rdi, [rip + uts_buf]
    syscall
    test rax, rax
    js .get_kernel_fallback
    mov rdi, r8
    lea rsi, [rip + uts_buf + UTS_RELEASE_OFF]
    mov edx, SMALL_BUF
    call copy_cstr_cap
    ret
.get_kernel_fallback:
    mov rdi, r8
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
    ret

# get_uptime:
#   T = open/read /proc/uptime + O(digits) parse + a few integer divisions.
#   Divisions are nanoseconds; procfs formatting dominates.
get_uptime:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov byte ptr [r12], 0

    lea rdi, [rip + path_uptime]
    lea rsi, [rip + small_buf]
    mov edx, 128
    call read_file
    test rax, rax
    jz .get_uptime_fallback

    lea rdi, [rip + small_buf]
    call parse_u64_dec
    mov r13, rax                # seconds

    mov rax, r13
    xor edx, edx
    mov ecx, 60
    div rcx
    mov r14, rax                # minutes

    mov rax, r14
    xor edx, edx
    mov ecx, 60
    div rcx
    mov r15, rax                # hours

    mov rax, r15
    xor edx, edx
    mov ecx, 24
    div rcx
    mov rbx, rax                # days
    mov r13, rdx                # hours % 24

    mov rax, r14
    xor edx, edx
    mov ecx, 60
    div rcx
    mov r14, rdx                # minutes % 60

    cmp rbx, 0
    je .get_uptime_no_days

    mov rdi, r12
    mov rsi, rbx
    call append_u64_field
    mov rdi, r12
    lea rsi, [rip + uptime_d]
    call append_into_field
    mov rdi, r12
    mov rsi, r13
    call append_u64_field
    mov rdi, r12
    lea rsi, [rip + uptime_h]
    call append_into_field
    mov rdi, r12
    mov rsi, r14
    call append_u64_field
    mov rdi, r12
    lea rsi, [rip + uptime_m]
    call append_into_field
    jmp .get_uptime_ret

.get_uptime_no_days:
    mov rdi, r12
    mov rsi, r15
    call append_u64_field
    mov rdi, r12
    lea rsi, [rip + uptime_h]
    call append_into_field
    mov rdi, r12
    mov rsi, r14
    call append_u64_field
    mov rdi, r12
    lea rsi, [rip + uptime_m]
    call append_into_field
    jmp .get_uptime_ret

.get_uptime_fallback:
    mov rdi, r12
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_uptime_ret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

# get_memory:
#   T = read /proc/meminfo + scan <=4 KiB once.
#   The math is:
#     used_kib = total_kib - avail_kib
#     hundredths_gib = floor(kib * 100 / 2^20)
get_memory:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov byte ptr [r12], 0

    lea rdi, [rip + path_meminfo]
    lea rsi, [rip + small_buf]
    mov edx, 4096
    call read_file
    test rax, rax
    jz .get_memory_fallback

    lea rbx, [rip + small_buf]
    mov rdi, rbx
    lea rsi, [rip + needle_memtotal]
    call find_substr
    test rax, rax
    jz .get_memory_fallback
    add rax, 9
    mov rsi, rax
.get_memory_skip_total:
    cmp byte ptr [rsi], ' '
    je .get_memory_st_next
    cmp byte ptr [rsi], 9
    jne .get_memory_parse_total
.get_memory_st_next:
    inc rsi
    jmp .get_memory_skip_total
.get_memory_parse_total:
    mov rdi, rsi
    call parse_u64_dec
    mov r13, rax

    mov rdi, rbx
    lea rsi, [rip + needle_memavail]
    call find_substr
    test rax, rax
    jz .get_memory_fallback
    add rax, 13
    mov rsi, rax
.get_memory_skip_avail:
    cmp byte ptr [rsi], ' '
    je .get_memory_sa_next
    cmp byte ptr [rsi], 9
    jne .get_memory_parse_avail
.get_memory_sa_next:
    inc rsi
    jmp .get_memory_skip_avail
.get_memory_parse_avail:
    mov rdi, rsi
    call parse_u64_dec
    mov r14, rax

    test r13, r13
    jz .get_memory_fallback
    mov r15, r13
    sub r15, r14

    mov rax, r15
    mov ecx, 100
    mul rcx
    add rax, 524288
    adc rdx, 0
    mov ecx, 1048576
    div rcx
    mov r15, rax

    mov rax, r13
    mov ecx, 100
    mul rcx
    add rax, 524288
    adc rdx, 0
    mov ecx, 1048576
    div rcx
    mov r13, rax

    mov rdi, r12
    mov rsi, r15
    call append_fixed2_field
    mov rdi, r12
    lea rsi, [rip + gib_sep]
    call append_into_field
    mov rdi, r12
    mov rsi, r13
    call append_fixed2_field
    mov rdi, r12
    lea rsi, [rip + gib_tail]
    call append_into_field
    jmp .get_memory_ret

.get_memory_fallback:
    mov rdi, r12
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_memory_ret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

get_wm:
    mov rsi, [rip + env_xdg]
    test rsi, rsi
    jnz .get_wm_copy
    mov rsi, [rip + env_dsess]
    test rsi, rsi
    jnz .get_wm_copy
    lea rsi, [rip + unknown_str]
.get_wm_copy:
    mov edx, SMALL_BUF
    call copy_cstr_cap
    ret

get_shell:
    mov r8, rdi
    mov rsi, [rip + env_shell]
    test rsi, rsi
    jz .get_shell_fallback
    mov rdi, rsi
    call basename_ptr
    mov rsi, rax
    mov rdi, r8
    mov edx, SMALL_BUF
    call copy_cstr_cap
    ret
.get_shell_fallback:
    mov rdi, r8
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
    ret

# get_terminal:
#   Fast path = TERM_PROGRAM env.
#   Slow path = chase parent process exe names through /proc/<pid>/{exe,stat}.
#   The arithmetic here is trivial; procfs path lookup and readlink dominate.
get_terminal:
    push rbx
    push r12
    push r13
    push r14
    mov r12, rdi

    mov rsi, [rip + env_termprog]
    test rsi, rsi
    jz .get_terminal_ppid_path
    mov rdi, r12
    mov edx, SMALL_BUF
    call copy_cstr_cap
    jmp .get_terminal_ret

.get_terminal_ppid_path:
    mov eax, SYS_getppid
    syscall
    mov r13, rax
    mov r14d, 2
.get_terminal_term_loop:
    lea rdi, [rip + tmp_path]
    mov rsi, r13
    lea rdx, [rip + proc_exe_suffix]
    call pid_to_path

    mov eax, SYS_readlinkat
    mov edi, AT_FDCWD
    lea rsi, [rip + tmp_path]
    lea rdx, [rip + small_buf]
    mov r10d, 511
    syscall
    test rax, rax
    jle .get_terminal_fallback
    lea r8, [rip + small_buf]
    mov byte ptr [r8 + rax], 0

    lea rdi, [rip + small_buf]
    call basename_ptr
    mov rbx, rax
    mov rdi, rbx
    call is_shell_like
    test eax, eax
    jz .get_terminal_use_name

    lea rdi, [rip + tmp_path]
    mov rsi, r13
    lea rdx, [rip + proc_stat_suffix]
    call pid_to_path
    lea rdi, [rip + tmp_path]
    lea rsi, [rip + small_buf]
    mov edx, 4096
    call read_file
    test rax, rax
    jz .get_terminal_fallback
    lea rdi, [rip + small_buf]
    call parse_ppid_from_stat
    test rax, rax
    jz .get_terminal_fallback
    mov r13, rax
    dec r14d
    jnz .get_terminal_term_loop
    jmp .get_terminal_fallback

.get_terminal_use_name:
    mov rdi, r12
    mov rsi, rbx
    mov edx, SMALL_BUF
    call copy_cstr_cap
    jmp .get_terminal_ret

.get_terminal_fallback:
    mov rsi, [rip + env_term]
    test rsi, rsi
    jnz .get_terminal_copy_term
    lea rsi, [rip + unknown_str]
.get_terminal_copy_term:
    mov rdi, r12
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_terminal_ret:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

# get_cpu:
#   T = 3 CPUID leaves + one tiny sysfs read + one tiny online/present scan.
#   Pure compute is a few dozen instructions; the only meaningful variable term
#   is the cpufreq sysfs read.
get_cpu:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov byte ptr [r12], 0

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000004
    jb .get_cpu_fallback

    lea r10, [rip + brand_buf]

    mov eax, 0x80000002
    cpuid
    mov dword ptr [r10], eax
    mov dword ptr [r10 + 4], ebx
    mov dword ptr [r10 + 8], ecx
    mov dword ptr [r10 + 12], edx

    mov eax, 0x80000003
    cpuid
    mov dword ptr [r10 + 16], eax
    mov dword ptr [r10 + 20], ebx
    mov dword ptr [r10 + 24], ecx
    mov dword ptr [r10 + 28], edx

    mov eax, 0x80000004
    cpuid
    mov dword ptr [r10 + 32], eax
    mov dword ptr [r10 + 36], ebx
    mov dword ptr [r10 + 40], ecx
    mov dword ptr [r10 + 44], edx
    mov byte ptr [r10 + 48], 0

    lea rsi, [rip + brand_buf]
.get_cpu_skip_leading:
    cmp byte ptr [rsi], ' '
    jne .get_cpu_copy_brand
    inc rsi
    jmp .get_cpu_skip_leading

.get_cpu_copy_brand:
    lea rdi, [rip + brand_clean]
    mov r15, rdi
    xor r8d, r8d               # previous char was space
.get_cpu_brand_loop:
    mov al, byte ptr [rsi]
    test al, al
    jz .get_cpu_brand_done
    cmp al, '@'
    je .get_cpu_brand_done
    cmp al, ' '
    jne .get_cpu_emit
    test r8b, r8b
    jnz .get_cpu_next_brand
    mov byte ptr [rdi], ' '
    inc rdi
    mov r8b, 1
    jmp .get_cpu_next_brand
.get_cpu_emit:
    mov byte ptr [rdi], al
    inc rdi
    xor r8d, r8d
.get_cpu_next_brand:
    inc rsi
    jmp .get_cpu_brand_loop
.get_cpu_brand_done:
    cmp rdi, r15
    je .get_cpu_brand_store
    cmp byte ptr [rdi - 1], ' '
    jne .get_cpu_brand_store
    dec rdi
.get_cpu_brand_store:
    mov byte ptr [rdi], 0

    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw1]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw2]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw3]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw4]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw5]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw6]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw7]
    call strip_keyword
    lea rdi, [rip + brand_clean]
    lea rsi, [rip + cpu_kw8]
    call strip_keyword

    call get_cpu_threads
    mov r13, rax

    lea rdi, [rip + path_cpu_freq]
    lea rsi, [rip + small_buf]
    mov edx, 64
    call read_file
    xor r14, r14
    test rax, rax
    jz .get_cpu_build_cpu
    lea rdi, [rip + small_buf]
    call parse_u64_dec
    mov r14, rax               # kHz

.get_cpu_build_cpu:
    mov rdi, r12
    lea rsi, [rip + brand_clean]
    call append_into_field
    mov rdi, r12
    lea rsi, [rip + cpu_open_paren]
    call append_into_field
    mov rdi, r12
    mov rsi, r13
    call append_u64_field
    cmp r14, 0
    jz .get_cpu_close_only

    mov rdi, r12
    lea rsi, [rip + cpu_close_at]
    call append_into_field
    mov rax, r14
    mov ecx, 100
    mul rcx
    add rax, 500000
    adc rdx, 0
    mov ecx, 1000000
    div rcx
    mov rdi, r12
    mov rsi, rax
    call append_fixed2_field
    mov rdi, r12
    lea rsi, [rip + cpu_ghz]
    call append_into_field
    jmp .get_cpu_ret

.get_cpu_close_only:
    mov rdi, r12
    lea rsi, [rip + cpu_close_paren]
    call append_into_field
    jmp .get_cpu_ret

.get_cpu_fallback:
    mov rdi, r12
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_cpu_ret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

# get_gpu:
#   T = one procfs directory walk over NVIDIA GPU slots + one small information
#   file read, else one sysfs uevent read.
#   On this machine the NVIDIA path is the intended fast path.
get_gpu:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov byte ptr [r12], 0

    lea rdi, [rip + path_nvidia_gpus]
    mov eax, SYS_open
    mov rsi, O_RDONLY | O_DIRECTORY | O_CLOEXEC
    xor edx, edx
    syscall
    test rax, rax
    js .get_gpu_fallback
    mov r13, rax

.get_gpu_dir_read:
    mov eax, SYS_getdents64
    mov edi, r13d
    lea rsi, [rip + dir_buf]
    mov edx, DIR_BUF
    syscall
    test rax, rax
    jle .get_gpu_close_fallback
    mov r14, rax
    xor rbx, rbx
.get_gpu_dir_loop:
    cmp rbx, r14
    jae .get_gpu_dir_read
    lea r8, [rip + dir_buf]
    add r8, rbx
    lea r9, [r8 + D_NAME_OFF]
    cmp byte ptr [r9], '.'
    je .get_gpu_next_ent

    mov eax, SYS_openat
    mov edi, r13d
    mov rsi, r9
    mov edx, O_RDONLY | O_DIRECTORY | O_CLOEXEC
    xor r10d, r10d
    syscall
    test rax, rax
    js .get_gpu_next_ent
    mov r15, rax

    mov eax, SYS_openat
    mov edi, r15d
    lea rsi, [rip + name_information]
    mov edx, O_RDONLY | O_CLOEXEC
    xor r10d, r10d
    syscall
    test rax, rax
    js .get_gpu_close_subdir
    mov r10, rax

    mov eax, SYS_read
    mov edi, r10d
    lea rsi, [rip + file_buf]
    mov edx, FILE_BUF - 1
    syscall
    test rax, rax
    jle .get_gpu_close_both
    lea r11, [rip + file_buf]
    mov byte ptr [r11 + rax], 0

    mov eax, SYS_close
    mov edi, r10d
    syscall
    mov eax, SYS_close
    mov edi, r15d
    syscall

    lea rdi, [rip + file_buf]
    lea rsi, [rip + needle_model]
    call find_substr
    test rax, rax
    jz .get_gpu_next_ent
    add rax, 6
.get_gpu_skip_model_ws:
    cmp byte ptr [rax], ' '
    je .get_gpu_sm_next
    cmp byte ptr [rax], 9
    jne .get_gpu_copy_model
.get_gpu_sm_next:
    inc rax
    jmp .get_gpu_skip_model_ws
.get_gpu_copy_model:
    mov rsi, rax
    xor rcx, rcx
.get_gpu_gpu_loop:
    cmp rcx, SMALL_BUF - 1
    jae .get_gpu_done_gpu
    mov dl, byte ptr [rsi + rcx]
    test dl, dl
    jz .get_gpu_done_gpu
    cmp dl, 10
    je .get_gpu_done_gpu
    mov byte ptr [r12 + rcx], dl
    inc rcx
    jmp .get_gpu_gpu_loop
.get_gpu_done_gpu:
    mov byte ptr [r12 + rcx], 0
    mov eax, SYS_close
    mov edi, r13d
    syscall
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.get_gpu_close_both:
    mov eax, SYS_close
    mov edi, r10d
    syscall
.get_gpu_close_subdir:
    mov eax, SYS_close
    mov edi, r15d
    syscall
    jmp .get_gpu_next_ent

.get_gpu_next_ent:
    movzx eax, word ptr [r8 + D_RECLEN_OFF]
    add rbx, rax
    jmp .get_gpu_dir_loop

.get_gpu_close_fallback:
    mov eax, SYS_close
    mov edi, r13d
    syscall

.get_gpu_fallback:
    lea rdi, [rip + path_card0_uevent]
    lea rsi, [rip + small_buf]
    mov edx, 4096
    call read_file
    test rax, rax
    jz .get_gpu_unknown

    lea rdi, [rip + small_buf]
    lea rsi, [rip + needle_driver]
    call find_substr
    test rax, rax
    jz .get_gpu_unknown
    add rax, 7
    mov rsi, rax
    xor rcx, rcx
.get_gpu_drv_loop:
    cmp rcx, SMALL_BUF - 1
    jae .get_gpu_drv_done
    mov dl, byte ptr [rsi + rcx]
    test dl, dl
    jz .get_gpu_drv_done
    cmp dl, 10
    je .get_gpu_drv_done
    mov byte ptr [r12 + rcx], dl
    inc rcx
    jmp .get_gpu_drv_loop
.get_gpu_drv_done:
    mov byte ptr [r12 + rcx], 0
    jmp .get_gpu_ret

.get_gpu_unknown:
    mov rdi, r12
    lea rsi, [rip + unknown_gpu_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_gpu_ret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

append_pkg_count:
    test rsi, rsi
    jz .append_pkg_count_ret
    push rbx
    push r12
    mov rbx, rdx
    mov r12, rdi
    mov rdi, rsi
    lea rsi, [rip + tmp_num]
    call u64_to_cstr
    mov rdi, r12
    lea rsi, [rip + tmp_num]
    call append_into_field
    mov rdi, r12
    mov rsi, rbx
    call append_into_field
    pop r12
    pop rbx
.append_pkg_count_ret:
    ret

# get_packages:
#   T = T_pacman_dirwalk + T_flatpak_dirwalks + T_nix_manifest_scans
#   This is the single largest exact-work term because it scales with package
#   database size instead of with a fixed tiny procfs file.
get_packages:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov byte ptr [r12], 0

    lea rdi, [rip + path_pacman]
    call count_dir_entries
    mov r13, rax

    lea rdi, [rip + path_flatpak_sys]
    call count_dir_entries
    mov r14, rax

    xor r15, r15
    lea rdi, [rip + path_nix_default]
    lea rsi, [rip + needle_active]
    call count_manifest_needle
    add r15, rax
    lea rdi, [rip + path_nix_system]
    lea rsi, [rip + needle_active]
    call count_manifest_needle
    add r15, rax

    lea rdi, [rip + tmp_path]
    lea rsi, [rip + suffix_flatpak_user]
    call build_home_path
    test eax, eax
    jz .get_packages_skip_user_flatpak
    lea rdi, [rip + tmp_path]
    call count_dir_entries
    add r14, rax
.get_packages_skip_user_flatpak:

    lea rdi, [rip + tmp_path]
    lea rsi, [rip + suffix_nix_profile_json]
    call build_home_path
    test eax, eax
    jz .get_packages_skip_user_nix_json
    lea rdi, [rip + tmp_path]
    lea rsi, [rip + needle_active]
    call count_manifest_needle
    add r15, rax
    test rax, rax
    jnz .get_packages_skip_user_nix_json
    lea rdi, [rip + tmp_path]
    lea rsi, [rip + suffix_nix_profile_nix]
    call build_home_path
    test eax, eax
    jz .get_packages_skip_user_nix_json
    lea rdi, [rip + tmp_path]
    lea rsi, [rip + needle_nameeq]
    call count_manifest_needle
    add r15, rax
.get_packages_skip_user_nix_json:

    lea rdi, [rip + tmp_path]
    lea rsi, [rip + suffix_nix_home_mgr]
    call build_home_path
    test eax, eax
    jz .get_packages_format
    lea rdi, [rip + tmp_path]
    lea rsi, [rip + needle_active]
    call count_manifest_needle
    add r15, rax

.get_packages_format:
    mov rdi, r12
    mov rsi, r13
    lea rdx, [rip + pkg_pacman_lbl]
    call append_pkg_count
    mov rdi, r12
    mov rsi, r14
    lea rdx, [rip + pkg_flatpak_lbl]
    call append_pkg_count
    mov rdi, r12
    mov rsi, r15
    lea rdx, [rip + pkg_nix_lbl]
    call append_pkg_count

    mov rdi, r12
    call cstr_len
    cmp rax, 2
    jb .get_packages_unknown
    mov byte ptr [r12 + rax - 2], 0
    jmp .get_packages_ret

.get_packages_unknown:
    mov rdi, r12
    lea rsi, [rip + unknown_str]
    mov edx, SMALL_BUF
    call copy_cstr_cap
.get_packages_ret:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

build_output:
    mov qword ptr [rip + out_off], 0

    lea rdi, [rip + art0]
    call append_cstr
    lea rdi, [rip + art1]
    call append_cstr
    lea rdi, [rip + art2]
    call append_cstr
    lea rdi, [rip + art3]
    call append_cstr
    lea rdi, [rip + art4]
    call append_cstr
    lea rdi, [rip + art5]
    call append_cstr
    lea rdi, [rip + art6]
    call append_cstr
    lea rdi, [rip + art7]
    call append_cstr
    lea rdi, [rip + art8]
    call append_cstr
    lea rdi, [rip + art9]
    call append_cstr
    lea rdi, [rip + art10]
    call append_cstr
    lea rdi, [rip + art11]
    call append_cstr
    lea rdi, [rip + art12]
    call append_cstr

    lea rdi, [rip + line_distro]
    lea rsi, [rip + distro_buf]
    call out_line_value
    lea rdi, [rip + line_kernel]
    lea rsi, [rip + kernel_buf]
    call out_line_value
    lea rdi, [rip + line_uptime]
    lea rsi, [rip + uptime_buf]
    call out_line_value
    lea rdi, [rip + line_wm]
    lea rsi, [rip + wm_buf]
    call out_line_value
    lea rdi, [rip + line_packages]
    lea rsi, [rip + pkg_buf]
    call out_line_value
    lea rdi, [rip + line_terminal]
    lea rsi, [rip + term_buf]
    call out_line_value
    lea rdi, [rip + line_memory]
    lea rsi, [rip + mem_buf]
    call out_line_value
    lea rdi, [rip + line_shell]
    lea rsi, [rip + shell_buf]
    call out_line_value
    lea rdi, [rip + line_cpu]
    lea rsi, [rip + cpu_buf]
    call out_line_value
    lea rdi, [rip + line_gpu]
    lea rsi, [rip + gpu_buf]
    call out_line_value

    lea rdi, [rip + art13]
    call append_cstr
    ret
