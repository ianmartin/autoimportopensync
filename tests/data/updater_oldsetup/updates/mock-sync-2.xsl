<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <xsl:import href="mock-sync-1.xsl"/>

  <xsl:template match="/">
    <xsl:apply-templates select="/" mode="mock-sync-2"/>
  </xsl:template>

  <xsl:template match="/" mode="mock-sync-2">
    <xsl:variable name="rtf">
      <!-- Select previous mode -->
      <xsl:apply-templates select="/" mode="mock-sync-1"/>
    </xsl:variable>
    <xsl:variable name="node" select="exslt:node-set($rtf)"/>
    
    <xsl:apply-templates select="$node/*" mode="mock-sync-2"/>
  </xsl:template>

  <xsl:template match="config" mode="mock-sync-2">
    <config><xsl:apply-templates mode="mock-sync-2"/></config>
  </xsl:template>

  <xsl:template match="config/*" mode="mock-sync-2">
    <directory><xsl:value-of select="." /></directory>
  </xsl:template>


</xsl:transform>
