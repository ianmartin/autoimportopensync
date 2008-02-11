<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <xsl:import href="syncgroup-2.xsl"/>

  <xsl:template match="/">
    <xsl:apply-templates select="/" mode="syncgroup-3"/>
  </xsl:template>

  <xsl:template match="/" mode="syncgroup-3">
    <xsl:variable name="rtf">
      <!-- Select previous mode -->
      <xsl:apply-templates select="/" mode="syncgroup-2"/>
    </xsl:variable>
    <xsl:variable name="node" select="exslt:node-set($rtf)"/>
    
    <xsl:apply-templates select="$node/*" mode="syncgroup-3"/>
  </xsl:template>

  <xsl:template match="syncgroup" mode="syncgroup-3">
    <syncgroup><xsl:apply-templates mode="syncgroup-3"/></syncgroup>
  </xsl:template>

  <xsl:template match="syncgroup/groupname" mode="syncgroup-3">
    <groupname><xsl:apply-templates mode="syncgroup-3"/></groupname>
  </xsl:template>

  <xsl:template match="syncgroup/versiontwo" mode="syncgroup-3">
    <versionthree><xsl:apply-templates mode="syncgroup-3"/></versionthree>
 </xsl:template>

</xsl:transform>
