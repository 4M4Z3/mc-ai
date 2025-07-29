#include "Block.h"

Block::Block() : m_type(BlockType::AIR) {
}

Block::Block(BlockType type) : m_type(type) {
}

BlockType Block::GetType() const {
    return m_type;
}

void Block::SetType(BlockType type) {
    m_type = type;
}

bool Block::IsAir() const {
    return m_type == BlockType::AIR;
}

bool Block::IsSolid() const {
    return m_type != BlockType::AIR;
} 