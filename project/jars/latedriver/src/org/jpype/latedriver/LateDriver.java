package org.jpype.latedriver;

import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverManager;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.util.Properties;
import java.util.logging.Logger;

/**
 * Minimal JDBC driver used to test that a driver jar added to the
 * classpath after the JVM has started is still picked up by
 * java.sql.DriverManager. Registers itself via a static initializer, as
 * required of every JDBC driver implementation (JDBC 4 Class.forName
 * loading convention), and is listed in META-INF/services/java.sql.Driver
 * for ServiceLoader-based discovery.
 */
public class LateDriver implements Driver
{
	static
	{
		try
		{
			DriverManager.registerDriver(new LateDriver());
		} catch (SQLException ex)
		{
			throw new RuntimeException(ex);
		}
	}

	@Override
	public Connection connect(String url, Properties info)
	{
		return null;
	}

	@Override
	public boolean acceptsURL(String url)
	{
		return url != null && url.startsWith("jdbc:latedriver:");
	}

	@Override
	public DriverPropertyInfo[] getPropertyInfo(String url, Properties info)
	{
		return new DriverPropertyInfo[0];
	}

	@Override
	public int getMajorVersion()
	{
		return 1;
	}

	@Override
	public int getMinorVersion()
	{
		return 0;
	}

	@Override
	public boolean jdbcCompliant()
	{
		return false;
	}

	@Override
	public Logger getParentLogger()
	{
		return null;
	}
}
