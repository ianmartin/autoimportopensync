<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <!-- No include. It's the first update stylesheet.
  <xsl:import href="syncgroup-0.xsl"/>
  -->
  
  <!-- Standard-Template -->
  <xsl:template match="*" mode="syncgroup-1">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates mode="syncgroup-1"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
  <!--
    <xsl:if test="function-available(exslt:node-set)">
      <xsl:message terminate="yes">
        <xsl:text>ERROR: No function exslt:node-set found!</xsl:text>
      </xsl:message>    
    </xsl:if>
    -->
    
    <xsl:apply-templates mode="syncgroup-1"/>
  </xsl:template>

  <xsl:template match="syncgroup" mode="syncgroup-1">
    <syncgroup><xsl:apply-templates mode="syncgroup-1"/></syncgroup>
  </xsl:template>

  <xsl:template match="syncgroup/groupname" mode="syncgroup-1">
    <groupname><xsl:apply-templates mode="syncgroup-1"/></groupname>
  </xsl:template>

  <xsl:template match="syncgroup/last_sync" mode="syncgroup-1">
    <versionone><xsl:value-of select="." /></versionone>
  </xsl:template>

</xsl:transform>
