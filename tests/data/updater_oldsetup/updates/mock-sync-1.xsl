<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <!-- No include. It's the first update stylesheet.
  <xsl:import href="mock-sync-0.xsl"/>
  -->
  
  <!-- Standard-Template -->
  <xsl:template match="*" mode="mock-sync-1">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates mode="mock-sync-1"/>
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
    
    <xsl:apply-templates mode="mock-sync-1"/>
  </xsl:template>

  <xsl:template match="config" mode="mock-sync-1">
    <config><xsl:apply-templates mode="mock-sync-1"/></config>
  </xsl:template>

  <xsl:template match="config/*" mode="mock-sync-1">
    <directory><xsl:value-of select="." /></directory>
  </xsl:template>

</xsl:transform>
