/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include "Node.hpp"
#include <iomanip>

namespace cuda
{

namespace JIT
{

    class UnaryNode : public Node
    {
    private:
        std::string m_op_str;
        Node_ptr m_child;
        int m_op;
        bool m_is_check;

    public:
        UnaryNode(const char *out_type_str, const char *name_str,
                  const std::string &op_str,
                  Node_ptr child, int op, bool is_check=false)
            : Node(out_type_str, name_str, child->getHeight() + 1),
              m_op_str(op_str),
              m_child(child),
              m_op(op),
              m_is_check(is_check)
        {
        }

        bool isLinear(dim_t dims[4])
        {
            if (!m_set_is_linear) {
                m_linear = m_child->isLinear(dims);
                m_set_is_linear = true;
            }
            return m_linear;
        }

        void genParams(std::stringstream &kerStream,
                       std::stringstream &annStream, bool is_linear)
        {
            if (m_gen_param) return;
            if (!(m_child->isGenParam())) m_child->genParams(kerStream, annStream, is_linear);
            m_gen_param = true;
        }


        void genOffsets(std::stringstream &kerStream, bool is_linear)
        {
            if (m_gen_offset) return;
            if (!(m_child->isGenOffset())) m_child->genOffsets(kerStream, is_linear);
            m_gen_offset = true;
        }

        void genKerName(std::stringstream &kerStream)
        {
            if (m_gen_name) return;

            m_child->genKerName(kerStream);

            // Make the hex representation of enum part of the Kernel name
            kerStream << "_" << std::setw(2) << std::setfill('0') << std::hex << m_op;
            kerStream << std::setw(2) << std::setfill('0') << std::hex << m_child->getId();
            kerStream << std::setw(2) << std::setfill('0') << std::hex << m_id << std::dec;
            m_gen_name = true;
        }

        void genFuncs(std::stringstream &kerStream, str_map_t &declStrs, bool is_linear)
        {
            if (m_gen_func) return;

            if (!(m_child->isGenFunc())) m_child->genFuncs(kerStream, declStrs, is_linear);

            std::stringstream declStream;

            if (m_is_check) {
                declStream << "declare " << "i32 " << m_op_str
                           << "(" << m_child->getTypeStr() << ")\n";
            } else {
                declStream << "declare " << m_type_str << " " << m_op_str
                           << "(" << m_child->getTypeStr() << ")\n";
            }

            str_map_iter loc = declStrs.find(declStream.str());
            if (loc == declStrs.end()) {
                declStrs[declStream.str()] = true;
            }

            if (m_is_check) {
                kerStream << "%tmp" << m_id << " = call i32 "
                          << m_op_str << "("
                          << m_child->getTypeStr() << " "
                          << "%val" << m_child->getId() << ")\n";

                if (m_type_str[0] == 'i') {
                    kerStream << "%val" << m_id << " = "
                              << "trunc i32 %tmp" << m_id << " to " << m_type_str << "\n";
                } else {
                    kerStream << "%val" << m_id << " = "
                              << "sitofp i32 %tmp" << m_id << " to " << m_type_str << "\n";
                }

            } else {
                kerStream << "%val" << m_id << " = call "
                          << m_type_str << " "
                          << m_op_str << "("
                          << m_child->getTypeStr() << " "
                          << "%val" << m_child->getId() << ")\n";
            }

            m_gen_func = true;
        }

        int setId(int id)
        {
            if (m_set_id) return id;
            id = m_child->setId(id);
            m_id = id;
            m_set_id = true;
            return m_id + 1;
        }

        void getInfo(unsigned &len, unsigned &buf_count, unsigned &bytes)
        {
            if (m_set_id) return;
            m_child->getInfo(len, buf_count, bytes);
            len++;
            m_set_id = true;
            return;
        }

        void resetFlags()
        {
            if (m_set_id) {
                m_is_check = false;
                resetCommonFlags();
                m_child->resetFlags();
            }
        }

        void setArgs(std::vector<void *> &args, bool is_linear)
        {
            if (m_set_arg) return;
            m_child->setArgs(args, is_linear);
            m_set_arg = true;
        }
    };

}

}
