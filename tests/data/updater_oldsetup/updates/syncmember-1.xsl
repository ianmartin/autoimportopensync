<?xml version="1.0" encoding="utf-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exslt="http://exslt.org/common"
  exclude-result-prefixes="exslt"
  version="1.0">

  <!-- No include. It's the first update stylesheet.
  <xsl:import href="syncmember-0.xsl"/>
  -->
  
  <!-- Standard-Template -->
  <xsl:template match="*" mode="syncmember-1">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates mode="syncmember-1"/>
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
    
    <xsl:apply-templates mode="syncmember-1"/>
  </xsl:template>

  <xsl:template match="syncmember" mode="syncmember-1">
    <syncmember><xsl:apply-templates mode="syncmember-1"/></syncmember>
  </xsl:template>

  <xsl:template match="syncmember/pluginname" mode="syncmember-1">
    <pluginname><xsl:apply-templates mode="syncmember-1"/></pluginname>
  </xsl:template>

  <xsl:template match="syncmember/name" mode="syncmember-1">
    <name><xsl:apply-templates mode="syncmember-1"/></name>
  </xsl:template>

</xsl:transform>
