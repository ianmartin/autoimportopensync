<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <xsl:import href="syncmember-1.xsl"/>

  <xsl:template match="/">
    <xsl:apply-templates select="/" mode="syncmember-2"/>
  </xsl:template>

  <xsl:template match="/" mode="syncmember-2">
    <xsl:variable name="rtf">
      <!-- Select previous mode -->
      <xsl:apply-templates select="/" mode="syncmember-1"/>
    </xsl:variable>
    <xsl:variable name="node" select="exslt:node-set($rtf)"/>
    
    <xsl:apply-templates select="$node/*" mode="syncmember-2"/>
  </xsl:template>

  <xsl:template match="syncmember" mode="syncmember-2">
    <syncmember><xsl:apply-templates mode="syncmember-2"/></syncmember>
  </xsl:template>

  <xsl:template match="syncmember/pluginname" mode="syncmember-2">
    <pluginname><xsl:apply-templates mode="syncmember-2"/></pluginname>
  </xsl:template>

  <xsl:template match="syncmember/name" mode="syncmember-2">
    <name><xsl:apply-templates mode="syncmember-2"/></name>
  </xsl:template>

</xsl:transform>
