<?xml version="1.0" ?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:atom="http://www.w3.org/2005/Atom"
		xmlns:gd="http://schemas.google.com/g/2005">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="child::entry">
  <contact>

	<xsl:for-each select="descendant::gd:postalAddress">
	  <Address>
	    <!-- TODO: add address location -->

	    <xsl:if test="@primary">
	      <xsl:attribute name="Pref">
		<xsl:value-of select="@primary"/>
              </xsl:attribute>
	    </xsl:if>

	    <ExtendedAddress>
	      <xsl:value-of select="."/>
	    </ExtendedAddress>
	  </Address>
	</xsl:for-each>

        <EMail>
	  <Content>
           <xsl:value-of select="descendant::gd:email/@address"/>
	  </Content>
        </EMail>

	<FormattedName>
	  <Content>
	    <xsl:value-of select="descendant::atom:title"/>
	  </Content>
	</FormattedName>

	<IM-AIM>
	  <Content>
	    <xsl:value-of select="descendant::gd:im[@protocol='http://schemas.google.com/g/2005#AIM']/@address"/>
	  </Content>
	</IM-AIM>
	<IM-ICQ>
	  <Content>
	    <xsl:value-of select="descendant::gd:im[@protocol='http://schemas.google.com/g/2005#ICQ']/@address"/>
	  </Content>
	</IM-ICQ>
	<IM-Jabber>
	  <Content>
	    <xsl:value-of select="descendant::gd:im[@protocol='http://schemas.google.com/g/2005#JABBER']/@address"/>
	  </Content>
	</IM-Jabber>
	<IM-MSN>
	  <Content>
	    <xsl:value-of select="descendant::gd:im[@protocol='http://schemas.google.com/g/2005#MSN']/@address"/>
	  </Content>
	</IM-MSN>
	<IM-Yahoo>
	  <Content>
	    <xsl:value-of select="descendant::gd:im[@protocol='http://schemas.google.com/g/2005#YAHOO']/@address"/>
	  </Content>
	</IM-Yahoo>
	<!-- Opensync doesn't have SKYPE type -->

	<Name>
	  <FirstName><xsl:value-of select="descendant::atom:title"/></FirstName>
	</Name>


	<Revision>
	  <Content>
	    <xsl:value-of select="descendant::atom:updated"/>
	  </Content>
	</Revision>


	<xsl:for-each select="descendant::gd:phoneNumber">
	  <Telephone>
 	    <Content>
	      <!-- Google allow use of @rel or @label, but not both! -->
	      <!-- TODO: parse @rel attribute and translate to valid Osync -->
<!-- 	      <xsl:attribute name="Location"> -->
<!-- 		<xsl:value-of select="@rel"/> -->
<!--               </xsl:attribute> -->
	      <!-- Sets the element text, the phone number -->
	      <xsl:value-of select="."/>
 	    </Content>
	  </Telephone>
	</xsl:for-each>

	<Uid>
	  <Content>
	    <xsl:value-of select="descendant::atom:id"/>
	  </Content>
	</Uid>

	<Url>
	  <Content>
	    <xsl:value-of select="descendant::atom:link[@rel='edit']/@href"/>
	  </Content>
	</Url>

  </contact>
</xsl:template>


</xsl:stylesheet>
