<?xml version="1.0" ?>
<xsl:stylesheet version="1.0"
		xmlns="http://www.w3.org/2005/Atom"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:gd="http://schemas.google.com/g/2005">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="child::contact">
<entry>

<!-- 	<id> -->
<!-- 	    <xsl:value-of select="descendant::Uid/Content"/> -->
<!-- 	</id> -->

<!-- 	<updated> -->
<!-- 	    <xsl:value-of select="descendant::Revision/Content"/> -->
<!-- 	</updated> -->

	<category>
	  <xsl:attribute name="scheme">http://schemas.google.com/g/2005#kind</xsl:attribute>
	  <xsl:attribute name="term">http://schemas.google.com/contact/2008#contact</xsl:attribute>
	</category>

	<title>
	  <xsl:attribute name="type">text</xsl:attribute>
	  <xsl:choose>
	    <xsl:when test="descendant::FormattedName/Content"><xsl:value-of select="descendant::FormattedName/Content"/></xsl:when>
	    <xsl:otherwise><xsl:value-of select="concat(descendant::Name/FirstName,
	    ' ', descendant::Name/LastName)"/></xsl:otherwise>
	  </xsl:choose>
	</title>

	<content><xsl:attribute name="type">text</xsl:attribute>
	  <xsl:value-of select="descendant::Title/Content"/>
	</content>

<!-- 	<link> -->
<!-- 	  <xsl:attribute name="rel">edit</xsl:attribute> -->
<!-- 	  <xsl:attribute name="type">application/atom+xml</xsl:attribute> -->
<!-- 	  <xsl:attribute name="href"> -->
<!-- 	    <xsl:value-of select="descendant::Url/Content"/> -->
<!-- 	  </xsl:attribute> -->
<!-- 	</link> -->

	<!-- TODO: handle type of email attribute -->
	<gd:email>
	  <xsl:attribute name="rel">http://schemas.google.com/g/2005#other</xsl:attribute>
	  <xsl:attribute name="address">
	    <xsl:value-of select="descendant::EMail/Content"/>
	  </xsl:attribute>
	</gd:email>


	<!-- TODO: add IM elements here -->


        <!-- TODO: label attributes -->
	<xsl:for-each select="descendant::Address">
	  <gd:postalAddress>
	    <xsl:attribute name="rel">http://schemas.google.com/g/2005#other</xsl:attribute>

	    <xsl:value-of select="."/>
	  </gd:postalAddress>
	</xsl:for-each>

	<xsl:for-each select="descendant::Telephone">
	  <gd:phoneNumber>

	    <xsl:if test="./@Type = 'Cellular'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#mobile</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	    <xsl:if test="./@Type = 'Voice'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#home</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	    <xsl:if test="./@Type = 'Fax'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#work_fax</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	    <xsl:if test="./@Type = 'Pager'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#pager</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	    <xsl:if test="./@Type = 'Company'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#work</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	    <xsl:if test="./@Type = 'Message'">
	      <xsl:attribute name="rel">http://schemas.google.com/g/2005#other</xsl:attribute>
	      <xsl:value-of select="./Content"/>
	    </xsl:if>

	  </gd:phoneNumber>
	</xsl:for-each>
</entry>

</xsl:template>


</xsl:stylesheet>
