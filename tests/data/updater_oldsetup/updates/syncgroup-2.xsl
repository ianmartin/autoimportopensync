<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <xsl:import href="syncgroup-1.xsl"/>

  <xsl:template match="/">
    <xsl:apply-templates select="/" mode="syncgroup-2"/>
  </xsl:template>

  <xsl:template match="/" mode="syncgroup-2">
    <xsl:variable name="rtf">
      <!-- Select previous mode -->
      <xsl:apply-templates select="/" mode="syncgroup-1"/>
    </xsl:variable>
    <xsl:variable name="node" select="exslt:node-set($rtf)"/>
    
    <xsl:apply-templates select="$node/*" mode="syncgroup-2"/>
  </xsl:template>

  <xsl:template match="syncgroup" mode="syncgroup-2">
    <syncgroup><xsl:apply-templates mode="syncgroup-2"/></syncgroup>
  </xsl:template>

  <xsl:template match="syncgroup/groupname" mode="syncgroup-2">
    <groupname><xsl:apply-templates mode="syncgroup-2"/></groupname>
  </xsl:template>

  <xsl:template match="syncgroup/versionone" mode="syncgroup-2">
<!--<versiontwo><xsl:apply-templates mode="syncgroup-2"/></versiontwo>-->
    <versiontwo><xsl:value-of select="." /></versiontwo>
 </xsl:template>

</xsl:transform>
